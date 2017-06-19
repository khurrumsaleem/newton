/*****************************************************************************\

NEWTON					      |
                      |
Implicit coupling 		|	TEST
in nonlinear			    |	PROGRAMM
calculations			    |
                      |

-------------------------------------------------------------------------------

This programm is a client code made for test Newton functionalities.
args:
 - arg[0]: bin name
 - arg[1]: problem number
 - arg[2]: client in the problem number
 - arg[3]: communication mode: ("io", "mpi_port", "mpi_comm")
 - arg[4]: communication arg: ("io": string of input file name)
                              ("mpi_port": int of code ID)
                              ("mpi_comm": string of communicator) ?

Author: Federico A. Caccia
Date: 17 June 2017

\*****************************************************************************/

#undef __FUNCT__
#define __FUNCT__ "main"

#include "Test.h"

int problemNumber;
int codeClient;
int codeID;
std::string comm;
std::string commArg;
int error=TEST_SUCCESS;
int order=CONTINUE;
int tag;
MPI_Comm Coupling_Comm;

using namespace::std;

int main(int argc,char **argv)
{    
  // Checking number of arguments
  if(argc!=5){
    cout<<"ERROR. Running test code with bad number of arguments."<<endl;
    return 0;
  }

  // Problem number
  stringstream(argv[1])>>problemNumber;
  // Client in this problem
  stringstream(argv[2])>>codeClient;
  // Communication mode
  comm = argv[3];
  if(comm != "io" && comm != "mpi_port" && comm != "mpi_comm"){
    cout<<"ERROR. Bad connection type"<<endl;
    return TEST_ERROR;
  }
  // Communication argument
  commArg = argv[4];

  switch(problemNumber){

    case TEST_2_LINEAR:
      linear2* system1;
      system1 = new linear2();
      try{
        system1->solve();
      }
      catch(int e){
        cout<<"ERROR running test."<<endl;
        return 0;
      }
      break;

    case TEST_2_NONLINEAR:
      nonlinear2* system2;
      system2 = new nonlinear2();
      try{
        system2->solve();
      }
      catch(int e){
        cout<<"ERROR running test."<<endl;
        return 0;
      }
      break;

    case TEST_3_LINEAR:
      linear3* system3;
      system3 = new linear3();
      try{
        system3->solve();
      }
      catch(int e){
        cout<<"ERROR running test."<<endl;
        return 0;
      }
      break;

    default:
      cout<<"ERROR. Bad problem number received in arg 1."<<endl;
      return 0;
  }

  //cout<<" Program finished succesfully"<<endl;  
  return TEST_SUCCESS;
}


/*-----------------------------------------------------------------------------

System to solve:

  1*w +  2*x +  3*y +  4*z = 17   (1a)
 12*w + 13*x + 14*y +  5*z = 18   (2a)
 11*w + 16*x + 15*y +  6*z = 19   (3a)
 10*w +  9*x +  8*y +  7*z = 20   (4a)

Code number 0 calculates (w,x,y) values as function of (z_guess) value readed from 
input, solving the coupled equations (1b,2b,3b):

 w = (17 -  2*x -  3*y - 4*z_guess)/ 1  (1b)
 x = (18 - 12*w - 14*y - 5*z_guess)/13  (2b)
 y = (19 - 11*w - 16*x - 6*z_guess)/15  (3b)

Code number 1 calculates (z) value as function of (w_guess,x_guess,y_guess) values 
readed from input, solving (4b):

 z = (20 - 10*w_guess -  9*x_guess - 8*y_guess)/ 7  (4b)

Analytical solution: 

 w = -0.70909
 x = -1.89091
 y = 2.36364
 z = 3.60000
-----------------------------------------------------------------------------*/

linear2::linear2()
{
  // Initialization
  switch(codeClient){
    case 0:
      input = new double[1];
      output = new double[3];
      mat = new double*[3];
      for(int i=0; i<3; i++){
        mat[i] = new double[3];
      }
      mat[0][0] = -0.439394;
      mat[0][1] = 0.272727;
      mat[0][2] = -0.166667;
      mat[1][0] = -0.393939;
      mat[1][1] = -0.272727;
      mat[1][2] = 0.333333;
      mat[2][0] = 0.742424;
      mat[2][1] = 0.090909;
      mat[2][2] = -0.166667;
      b = new double[3];
      break;
    case 1:
      input = new double[3];
      output = new double[1];
      b = new double[1];
      break;
    default:
      cout<<"ERROR. Bad client number received in arg 2."<<endl;
      throw TEST_ERROR;
  }
  if(comm=="io"){
    file = commArg;
    fileInput = file+".dat";
    fileOutput = file+".out";  
  }
  else if(comm=="mpi_port"){
    stringstream(commArg) >> codeID;
    // Connection
    mpi_connection();
    // Receiving control instruction
    error = mpi_receive_order();
    
    cout<<"First order: "<<order<<endl;
    if(order!=CONTINUE){    
      cout<<"Fatal error. Aborting."<<endl;
      mpi_finish();
      throw TEST_ERROR;
    }    
  }
}

void linear2::solve()
{
  do{
    switch(codeClient){
      case 0:
        // Load guess
        if(comm=="io"){                
          input = loaddata(fileInput, 1);
          z = input[0];
        }
        else if(comm=="mpi_port"){
          mpi_receive(input, 1);
          z = input[0];          
        }
        else if(comm=="mpi_comm"){
          
        }
        // Set b values
        b[0] = 17 - 4*z;
        b[1] = 18 - 5*z;
        b[2] = 19 - 6*z;
        // Solve system
        w = mat[0][0] * b[0] + mat[0][1] * b[1] + mat[0][2] * b[2];
        x = mat[1][0] * b[0] + mat[1][1] * b[1] + mat[1][2] * b[2];
        y = mat[2][0] * b[0] + mat[2][1] * b[1] + mat[2][2] * b[2];
        // Set solution
        output[0] = w;
        output[1] = x;
        output[2] = y;
        // Send results
        if(comm=="io"){                
          printResults(output, 3, fileOutput);
        }
        else if(comm=="mpi_port"){
          mpi_send(output, 3);
          order = mpi_receive_order();         
        }
        else if(comm=="mpi_comm"){         
          
        }
        
        break;
        
      case 1:
        // Load guess
        if(comm=="io"){
          input = loaddata(fileInput, 3);
          w = input[0];
          x = input[1];
          y = input[2];
        }
        else if(comm=="mpi_port"){
          mpi_receive(input, 3);
          w = input[0];
          x = input[1];
          y = input[2];   
        }
        else if(comm=="mpi_comm"){
          
        }
        // Set b values
        b[0] = 20.0/7;
        // Solve system
        z = b[0] - 10.0/7*w - 9.0/7*x - 8.0/7*y;
        // Set solution
        output[0] = z;
        // Send results
        if(comm=="io"){
          printResults(output, 1, fileOutput);
        }
        else if(comm=="mpi_port"){
          mpi_send(output, 1);
          order = mpi_receive_order();         
        }
        else if(comm=="mpi_comm"){
          
        }
        break;
        
      default:
        cout<<"ERROR. Bad client number received in arg 2."<<endl;
        throw TEST_ERROR;
    }
  }while(order==RESTART);
  
  if(order==ABORT){
    mpi_finish();
    cout<<"Finishing program by ABORT order"<<endl;
    throw TEST_ERROR;
  }
  
  // Finish connections
  if(comm=="mpi_port"){
    mpi_finish();
  }
}


/*-----------------------------------------------------------------------------

System to solve:

  1*w +  2*x +  3*y +  4*z = 17   (1a)
 12*w + 13*x + 14*y +  5*z = 18   (2a)
 11*w + 16*x + 15*y +  6*z = 19   (3a)
 10*w +  9*x +  8*y +  7*z = 20   (4a)

Code number 0 calculates (w,x) values as function of (y_guess, z_guess) value 
readed from input, solving the coupled equations (1b,2b):

 w = (17 -  2*x -  3*y_guess - 4*z_guess)/ 1  (1b)
 x = (18 - 12*w - 14*y_guess - 5*z_guess)/13  (2b)
 
Code number 1 calculates (y) value as function of (w_guess, x_guess, z_guess) 
values readed from input, solving (3b):

  y = (19 - 11*w_guess - 16*x_guess - 6*z_guess)/15  (3b)

Code number 1 calculates (z) value as function of (w_guess, x_guess, y_guess) 
values readed from input, solving (4b):

 z = (20 - 10*w_guess -  9*x_guess - 8*y_guess)/ 7  (4b)

Analytical solution: 

 w = -0.70909
 x = -1.89091
 y = 2.36364
 z = 3.60000
-----------------------------------------------------------------------------*/

linear3::linear3()
{
  // Initialization
  switch(codeClient){
    case 0:
      input = new double[1];
      output = new double[2];
      mat = new double*[2];
      for(int i=0; i<2; i++){
        mat[i] = new double[2];
      }
      mat[0][0] = -0.43478;
      mat[0][1] = 0.086957;
      mat[1][0] = 0.521739;
      mat[1][1] = -0.43478;
      b = new double[2];
      break;
      
    case 1:
      input = new double[3];
      output = new double[1];
      b = new double[1];
      break;
      
    case 2:
      input = new double[3];
      output = new double[1];
      b = new double[1];
      break;
      
    default:
      cout<<"ERROR. Bad client number received in arg 2."<<endl;
      throw TEST_ERROR;
  }
  if(comm=="io"){
    file = commArg;
    fileInput = file+".dat";
    fileOutput = file+".out";  
  }
  else if(comm=="mpi_port"){
    stringstream(commArg) >> codeID;
    // Connection
    mpi_connection();
    // Receiving control instruction
    error = mpi_receive_order();
    
    cout<<"First order: "<<order<<endl;
    if(order!=CONTINUE){    
      cout<<"Fatal error. Aborting."<<endl;
      mpi_finish();
      throw TEST_ERROR;
    }    
  }
}

void linear3::solve()
{
  do{
    switch(codeClient){
      case 0:
        // Load guess
        if(comm=="io"){                
          input = loaddata(fileInput, 2);
          y = input[0];
          z = input[1];
        }
        else if(comm=="mpi_port"){
          mpi_receive(input, 1);
          z = input[0];          
        }
        else if(comm=="mpi_comm"){
          
        }
        // Set b values
        b[0] = 17 -  3*y - 4*z;
        b[1] = 18 - 14*y - 5*z;
        // Solve system
        w = mat[0][0] * b[0] + mat[0][1] * b[1];
        x = mat[1][0] * b[0] + mat[1][1] * b[1];
        // Set solution
        output[0] = w;
        output[1] = x;
        // Send results
        if(comm=="io"){                
          printResults(output, 2, fileOutput);
        }
        else if(comm=="mpi_port"){
          mpi_send(output, 3);
          order = mpi_receive_order();         
        }
        else if(comm=="mpi_comm"){         
          
        }
        
        break;
        
      case 1:
        // Load guess
        if(comm=="io"){
          input = loaddata(fileInput, 3);
          w = input[0];
          x = input[1];
          z = input[2];
        }
        else if(comm=="mpi_port"){
          mpi_receive(input, 3);
          w = input[0];
          x = input[1];
          z = input[2];   
        }
        else if(comm=="mpi_comm"){
          
        }
        // Set b values
        b[0] = 19.0/15;
        // Solve system
        y = b[0] - 11.0/15*w - 16.0/15*x - 6.0/15*z;
        // Set solution
        output[0] = y;
        // Send results
        if(comm=="io"){
          printResults(output, 1, fileOutput);
        }
        else if(comm=="mpi_port"){
          mpi_send(output, 1);
          order = mpi_receive_order();         
        }
        else if(comm=="mpi_comm"){
          
        }
        break;
        
      case 2:
        // Load guess
        if(comm=="io"){
          input = loaddata(fileInput, 3);
          w = input[0];
          x = input[1];
          y = input[2];
        }
        else if(comm=="mpi_port"){
          mpi_receive(input, 3);
          w = input[0];
          x = input[1];
          y = input[2];   
        }
        else if(comm=="mpi_comm"){
          
        }
        // Set b values
        b[0] = 20.0/7;
        // Solve system
        z = b[0] - 10.0/7*w - 9.0/7*x - 8.0/7*y;
        // Set solution
        output[0] = z;
        // Send results
        if(comm=="io"){
          printResults(output, 1, fileOutput);
        }
        else if(comm=="mpi_port"){
          mpi_send(output, 1);
          order = mpi_receive_order();         
        }
        else if(comm=="mpi_comm"){
          
        }
        break;
        
      default:
        cout<<"ERROR. Bad client number received in arg 2."<<endl;
        throw TEST_ERROR;
    }
  }while(order==RESTART);
  
  if(order==ABORT){
    mpi_finish();
    cout<<"Finishing program by ABORT order"<<endl;
    throw TEST_ERROR;
  }
  
  // Finish connections
  if(comm=="mpi_port"){
    mpi_finish();
  }
}

nonlinear2::nonlinear2()
{

}

void nonlinear2::solve()
{

}

/* loaddata
Load variables from input file.

input: string file, n values to load
output: data loaded in vector

*/
double* loaddata(string file, int n)
{
  // Values to load
  double* values = new double[n];
  // Input file
  ifstream inputFile;
  
  inputFile.open(file.c_str());
	if (inputFile.is_open()){
    for(int i=0; i<n; i++){
      inputFile >> values[i];
    }
	}
	else{
		cout<<"ERROR reading input file: "<<file<<" from code client: "<<codeClient<<endl;
	}
  inputFile.close();
  
  return values;
}

/* printResults
Print variables in output file.

input: data in vector, size of vector, output string file
output: -

*/
void printResults(double* vec, int n, string file)
{
  // Output file
  ofstream outputFile;
  outputFile.precision(numeric_limits<double>::digits10 + 1);
  
  outputFile.open(file.c_str());
	if (outputFile.is_open()){
		for(int i=0; i<n; i++){      
      outputFile <<vec[i]<<endl;
    }
	}
	else{
		cout<<"ERROR writing output file: "<< file<<" from code client: "<<codeClient<<endl;
	}
  outputFile.close();  
}

/* mpi_connection



*/

void mpi_connection()
{
  char Port_Name[MPI_MAX_PORT_NAME];
  
  MPI_Init(NULL, NULL);
  
  // codeID is the code ID
  cout<<" MPI connection with code: "<<codeID<<endl;
  // Service Name is constructed in base of code ID codeID
  string Srvc_NameCPP = "Newton-" + int2str(codeID);
  char* Srvc_Name=(char*)Srvc_NameCPP.c_str();
  cout<<" Looking for service: "<< Srvc_Name<<endl;
  // Connection tries with master code via Coupling_Comm MPI communicator
  int nTries = 0;
  while (nTries < 5){
    error = MPI_Lookup_name (Srvc_Name, MPI_INFO_NULL, Port_Name);
    if (error != 0) {
      cout<<" Can’t find service, trying again"<<endl;
    }
    else{
      break;
    }
    nTries++;
  }
  if (error == 0){
    cout<<" Service found at port: "<<Port_Name<<" after "<< nTries+1<<" tries."<<endl;
    nTries = 0;
  }
  while (nTries < 5){
    cout<<" Connecting..."<<endl;
    error = MPI_Comm_connect(Port_Name, MPI_INFO_NULL, 0, MPI_COMM_SELF, &Coupling_Comm);
    if (error != 0){
      cout<<" Can’t connect to service, re-trying..."<<endl;
    }
    else{
      break;
    }
    nTries = nTries + 1;
  }
  
  if (error != 0){
    cout<<" ERROR: "<<error<<" connecting to server."<<endl;
    throw TEST_ERROR;
  }
  cout<<" ...connected."<<endl;
}

void mpi_receive(double* input, int n)
{
  // Values reception
  cout<<"Receiving "<<n<<" values..."<<endl;
  error = MPI_Recv (input, n, MPI_DOUBLE_PRECISION, 0, MPI_ANY_TAG, Coupling_Comm, MPI_STATUS_IGNORE);
  if (error != 0){
    cout<<"Error receiving data"<<endl;
    throw TEST_ERROR;
  }
  cout<<"Values received."<<endl;
}

int mpi_receive_order()
{
  // Order reception
  cout<<"Receiving order..."<<endl;
  error = MPI_Recv (&order, 1, MPI_INTEGER, 0, MPI_ANY_TAG, Coupling_Comm, MPI_STATUS_IGNORE);
  if (error != 0){
    cout<<"Error receiving order"<<endl;
    throw TEST_ERROR;
  }
  cout<<"Order received: "<<order<<"."<<endl;
  
  return order;
}

void mpi_send(double* output, int n)
{
  // Sending values
  tag = 0;
  cout<<"Sending values..."<<endl;
  error = MPI_Send (output, n, MPI_DOUBLE_PRECISION, 0, tag, Coupling_Comm);
  if (error != 0){
    cout<<"Error sending values"<<endl;
    throw TEST_ERROR;
  }
  cout<<"Values sent."<<endl;
}

void mpi_finish()
{
  // Disconnecting
  cout<<"Disconnecting..."<<endl;
  error = MPI_Comm_disconnect(&(Coupling_Comm));
  if (error != 0){
    cout<<"Error disconnecting"<<endl;
    throw TEST_ERROR;
  }
  cout<<"Finalizing MPI..."<<endl;
  MPI_Finalize();
}

/* int2str

Converts integers to strings.

input: int to print
output: string

*/
string int2str (int a)
{
  char str[10];
  sprintf(str, "%d", a);
  string myStr(str);
  return myStr;  
}