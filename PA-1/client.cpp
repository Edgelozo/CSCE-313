/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name:Jackson Garner
	UIN: 132002661
	Date: 9/22/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;

	char m_size[] = "256"; 
	char* m_buff = m_size; //creates a string version of MAX_MESSAGE
	//char* m_buff = (char*)MAX_MESSAGE;

	string channel_name = "control";
	vector<FIFORequestChannel*> channels;


	//control logic
	bool p_bool = false;
	bool t_bool = false;
	bool e_bool = false;
	bool f_bool = false;
	bool c_bool = false;

	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) { //proccesses the command line
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				p_bool = true;
				break;
			case 't':
				t = atof (optarg);
				t_bool = true;
				break;
			case 'e':
				e = atoi (optarg);
				e_bool = true;
				break;
			case 'f':
				filename = optarg;
				f_bool = true;
				break;
			case 'm':
				m_buff = optarg;
				break;
			case 'c':
				c_bool = true;
				break;
		}
	}

	/*
	Commands that could be run:
	./client -p 10 -t 59.004 -e 2
	./client -p 2
	./client -m 5000
	./client -f 10.csv

	IMPORTANT
	./client -p <patient no> -t <time in seconds> -e <ecg number> //gets a single data point
	./client -p <patient no> //gets many data points
	./client -f <file name> //requests a file
	./client -m <new buff size> //creates a new buffer size

	
	*/

	//HERE IS WHERE WE FORK TO SERVER ----------------------------
	pid_t pid = fork(); //Fork the proccess
	if (pid == 0){ //if child proccess, branch to server.cpp
		char server_path[] = {"./server"};
		char param[] = "-m"; //NOT HOW IT WORKS, if we pass -m with no parameters, getopt will not make a change (i think!)
		char* args[] = {server_path, param, m_buff, NULL};
		execvp(args[0], args); //child run
	}
	//---------------------------------------------------------------

	FIFORequestChannel chan1(channel_name, FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&chan1);

	if (c_bool == true)
	{
		MESSAGE_TYPE nc = NEWCHANNEL_MSG; //step 1 = create new channel requiest
		chan1.cwrite(&nc, sizeof(MESSAGE_TYPE)); //sends a new channel request

		char buf_response[30]; //step 2: server responds with 30 char array
		chan1.cread(&buf_response, sizeof(buf_response));

		channel_name = string(buf_response); //step 3: use response as name for new channel
		FIFORequestChannel* chan2 = new FIFORequestChannel(channel_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(chan2);
		//^^replaces chan_control to chan_new_channel name for all request
	} 

	FIFORequestChannel chan = *(channels.back());

	//cout << channel_name << endl;

	if (p_bool == true) //if its a data point request
	{
		if (t_bool == true && e_bool == true) //if single data point request 
		{
			char buf[MAX_MESSAGE];
			datamsg x(p, t, e);
			memcpy(buf, &x, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg));
			double reply;
			chan.cread(&reply, sizeof(double));
			cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
		}
		else //if its a multi data point request
		{
			//requesting 1000 data points, both ecg1 and ecg2
			ofstream thousand_csv;
            thousand_csv.open("received/x1.csv");
			double ecg1_val, ecg2_val;
			for (double i = 0; i < 4; i += 0.004) //Ask for the first 1000 data points
			{
				datamsg ecg1_request(p, i, 1);
				datamsg ecg2_request(p, i, 2);

				chan.cwrite(&ecg1_request, sizeof(datamsg));
				chan.cread(&ecg1_val, sizeof(double));

				chan.cwrite(&ecg2_request, sizeof(datamsg));
				chan.cread(&ecg2_val, sizeof(double));

				thousand_csv << i << "," << ecg1_val << "," << ecg2_val << endl;
			}
			thousand_csv.close();
		}

	}
	else if (f_bool == true) //if its a file transfer request
	{
			filemsg fm(0,0); //create a file message 
			int file_size = sizeof(filemsg) + (filename.size()+1); //buffer size to be sent = size of struct + filename + 1 (null) (cstring)
			int current_buff_size = atoi(m_buff);

			char* buf2 = new char[file_size];
			memcpy(buf2, &fm, sizeof(filemsg));
			strcpy(buf2 + sizeof(filemsg), filename.c_str()); //creates the buffer and fills it with fm, then name of file

			chan.cwrite(buf2, file_size); //writes the buffer using filemsg
			__int64_t file_size_response = 0; 
			chan.cread(&file_size_response, sizeof(__int64_t)); //gets the size of the file from server

			int number_of_messages = ceil((double)file_size_response / current_buff_size); //divides the size of the file by our picked buffer size, getting number of messages to be sent
			//cout << "number of messages " << number_of_messages << endl;
			
			//file creation
			string file_name = "received/" + string(filename);
			ofstream file_transfer(file_name); //need to make sure it works

			filemsg* fm_ptr = (filemsg*)buf2; //use old buffer instead of new one by treating as pointer
			char* chunk_buffer = new char[current_buff_size];

			int last_chunk_size = file_size_response % current_buff_size; //modulo for last chunk size
			if (last_chunk_size == 0) {last_chunk_size = current_buff_size;} //if its zero (somehow), then it evenly divides into the chunk size, so message length should be current buff size

			for (int i = 0; i < number_of_messages; i++) {
					fm_ptr->offset = i * current_buff_size; //works
					
					if (i == number_of_messages - 1) { //if i is 0 and number_of_messages is 1, then make sure to send size of that message
						fm_ptr->length = last_chunk_size; //if last message, turn length request to that size
					} else {
						fm_ptr->length = current_buff_size; //otherwise, make it max size
					}
					
					chan.cwrite(buf2, file_size);
					chan.cread(chunk_buffer, fm_ptr->length);
					file_transfer.write(chunk_buffer, fm_ptr->length);
				}
				file_transfer.close();
				delete[] chunk_buffer;
				delete[] buf2;
			
	}

	
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));
	if (c_bool == true)
	{
		FIFORequestChannel* chan2 = channels.back();
		channels.pop_back();
		chan = *(channels.back());
		MESSAGE_TYPE m2 = QUIT_MSG;
		chan.cwrite(&m2, sizeof(MESSAGE_TYPE));
		delete chan2;
	}
	//wait(NULL);
}
