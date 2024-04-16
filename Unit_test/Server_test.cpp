#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fstream>
#include <thread>

using namespace std;

class CppThread { 					//create a thread wrapper for futhur use

public:
	inline void start() {
		uthread = std::thread(&CppThread::run, this);
	}

	inline void join() {
		uthread.join();
	}

protected:
	virtual void run() = 0;	

private:
	// pointer to the thread
	std::thread uthread;
};

class Server{				// server class function to deal with the 
private:
    char bff[13] ={0};
    char* rddmsg[13]={0};
    int bffsize = 13;
public:
   
    int clt_soc;

    int byteno(){
        return read(clt_soc, bff,12);	//read the number of bytes from socket
    }
    
    void setupServer(int& server_fd, struct sockaddr_in& address, int port) { //setup server by server ip address & port number
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        std::cout<<"Socket creation failed"<<std::endl;
        exit(EXIT_FAILURE);
    } else if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) != 0)
    {
        std::cout<<"Socket creation succeed!!"<<std::endl;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    std::cout<<"After socket creation"<<std::endl;

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cout<<"Bind failed"<<std::endl;
        exit(EXIT_FAILURE);
    } else if(bind(server_fd, (struct sockaddr *)&address, sizeof(address)) >= 0)
    {
        std::cout<<"Bind success"<<std::endl;
    }

    if (listen(server_fd, 3) < 0) {
        std::cout<<"Listen"<<std::endl;
        exit(EXIT_FAILURE);
    }}

    void acceptConnection(int server_fd, struct sockaddr_in& address) {		// accept the connection request from client in the socket-clt_soc
    int addrlen = sizeof(address);
     clt_soc = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    std::cout<<"after socket accept"<<endl;
    if (clt_soc > 0)
    {
        std::cout<<"Accept"<<std::endl;
        //exit(EXIT_FAILURE);
    }

    }

    char* readctmsg(){								//read the message sent from client and store it in the character array pointer buffer
        char* buffer = new char[bffsize];
        memset(buffer, 0, bffsize);
        ssize_t bytesRead = read(clt_soc,buffer,bffsize);
       // Check if read was successful
        if (bytesRead < 0) {
        // Error handling
            std::cerr << "Error reading from socket" << std::endl;
            delete[] buffer; // Free allocated memory
            return nullptr;
        } else if (bytesRead == 0) {
        // Connection closed
            std::cerr << "Connection closed by peer" << std::endl;
            delete[] buffer; // Free allocated memory
            return nullptr;
        } else {
        // Read successful
            return buffer;
        }
    }

    void sendtrafsig(bool tfsig){			//send traffic signal to client depending on the signal , which is altered by the thread Trafficsig
        char tfmsg[6]={0} ;
        if (tfsig==0)
        {
            strcpy(tfmsg, "RED  ");
             write(clt_soc, tfmsg, 6);
        }else
        { 
            strcpy(tfmsg, "GREEN");
            write(clt_soc, tfmsg, 6);

        }
       
    }
};


class Readmsg: public CppThread{  // thread keep reading the message from the client 
    private:

    Server& ser; 

    void run(){
                

        while(1)
        {   if(ser.byteno()!=0) //detect whether there is message
            {
            
            strcpy(rdmsg, ser.readctmsg()); //copy the msg to the public character array rdmsg
            std::cout<<rdmsg<<std::endl; //prinout the msg

            if (rdmsg[3]=='n'){ // detect whether the 3rd element of rdmsg is 'n', which comes from the message "Entered" 
               trigger=1; 
            }else if(rdmsg[3]=='x') // detect whether the 3rd element of rdmsg is 'x', which comes from the message "Entered" 
            {   
                trigger=0;
            }        
        
        }
        std::this_thread::sleep_for(500ms);
    };
}
public:
 Readmsg(Server& serversig):ser(serversig){};
bool trigger;
char rdmsg[9]={0};
};

class Trafficsig: public CppThread{ // thread that generates the traffic light signal
 private:
    Readmsg& rdmsg;
    Server& ser;
    bool num;

 void run(){		// generates the traffic light signal num , starting from 0
     num = 0;
    while(1){
        if(strcmp(rdmsg.rdmsg,"Entered")==0)
        {
         num = !num;
         std::cout<<"The traffic signal is "<<num<<std::endl;
          std::this_thread::sleep_for(500ms);
          ser.sendtrafsig(num);
        }
        std::this_thread::sleep_for(5000ms);
    }
 }   
public:

Trafficsig(Readmsg& Rdmsg, Server& svr) : rdmsg(Rdmsg), ser(svr), num(false) {}
};


int main() {
    int server_fd;
    struct sockaddr_in address;
    int port = 5560;
    char keyin = 0;
    Server server;
    Readmsg rdmsg(server);
    Trafficsig trafsig(rdmsg,server);

    server.setupServer(server_fd, address, port);
    std::cout<<"aaa"<<std::endl;
    server.acceptConnection(server_fd, address);
    trafsig.start();
    rdmsg.start();
    trafsig.join();
    rdmsg.join();
    std::cout<<"End connection?(y/n)"<<std::endl;
    std::cin>>keyin;
    if(keyin=='y')
       { close(server.clt_soc);
        close(server_fd);
       }
    
return 0;
}