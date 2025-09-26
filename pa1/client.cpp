/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Jackson
	UIN: 333006680
	Date: 9/26/25
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <sys/wait.h>
#include <fstream>
#include <vector>
#include <algorithm>

using namespace std;


int main (int argc, char *argv[]) {
	int opt;
	int p = -1;
	double t = -1;
	int e = -1;
	int m = MAX_MESSAGE;
	bool new_chan = false;
	string filename = "";
	vector<FIFORequestChannel*> channels;

	while ((opt = getopt(argc, argv, "p:t:e:f:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				break;
			case 'e':
				e = atoi (optarg);
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm':
				m = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}
	pid_t pid = fork();
	if(pid<0){
		exit(1);
	}
	if(pid==0){ //child / server process
		string m_str = to_string(m);
		char* args[] = {(char*)"./server", (char*)"-m", (char*)m_str.c_str(), NULL};

		execvp(args[0], args);

		perror("Failed to execvp");
		exit(1);
	}
	
    FIFORequestChannel control_chan("control", FIFORequestChannel::CLIENT_SIDE);
	channels.push_back(&control_chan);

	if(new_chan){
		MESSAGE_TYPE nc = NEWCHANNEL_MSG;
		control_chan.cwrite(&nc, sizeof(MESSAGE_TYPE));

		char new_chan_name[100];
		control_chan.cread(new_chan_name, sizeof(new_chan_name));
		
		FIFORequestChannel* data_chan = new FIFORequestChannel(new_chan_name, FIFORequestChannel::CLIENT_SIDE);
		channels.push_back(data_chan);
	}
	FIFORequestChannel chan = *(channels.back());
	// single data point, only run p, t, e != -1
	// example data point request
	if(p != -1 && t != -1 && e != -1){
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);
		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		double reply;
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
	}
	// else, if p != -1, request 1000 data points
	// loop over first 1000 lines
	// send request for ecg 1
	// send request for ecg 2
	// write line to received x1.csv
	else if(p != -1){
		ofstream output_file("received/x1.csv");
		for(int i = 0; i < 1000; ++i){
			double time = i * .004;

			datamsg ecg1(p, time, 1);
			chan.cwrite(&ecg1, sizeof(datamsg));
			double reply1;
			chan.cread(&reply1, sizeof(double));

			datamsg ecg2(p, time, 2);
			chan.cwrite(&ecg2, sizeof(datamsg));
			double reply2;
			chan.cread(&reply2, sizeof(double));

			output_file << time << "," << reply1 << "," << reply2 << endl;
		}
		output_file.close();
	}
	// file request
	else if (filename != ""){
		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;
		int64_t filesize = 0;
		chan.cread(&filesize, sizeof(int64_t));

		ofstream output_file("received/" + fname, ios::binary);
		char* buf3 = new char[m]; // This is the buffer for receiving data

		filemsg* file_req = (filemsg*)buf2;
		for (int64_t offset = 0; offset < filesize; offset += m) {			
			int chunk_len = min((int64_t)m, filesize - offset);
			
			file_req->offset = offset;
			file_req->length = chunk_len;

			chan.cwrite(buf2, len);
			chan.cread(buf3, chunk_len);
			output_file.write(buf3, chunk_len);
		}

		output_file.close();
		delete[] buf2;
		delete[] buf3;
	}

	// closing the channel    
    MESSAGE_TYPE quit_msg = QUIT_MSG;
	for (FIFORequestChannel* chan : channels) {
		chan->cwrite(&quit_msg, sizeof(MESSAGE_TYPE));
	}

	for (size_t i = 1; i < channels.size(); ++i) {
		delete channels[i];
	}

	wait(NULL);
}
