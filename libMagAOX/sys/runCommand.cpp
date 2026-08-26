/** \file runCommand.cpp
  * \brief Run a command get the output.
  * \author Jared R. Males (jaredmales@gmail.com)
  *
  * \ingroup sys_files
  */

#include "runCommand.hpp"

#include <cstring>
#include <sstream>

#include <unistd.h>
#include <sys/wait.h>
#include <iostream>

namespace MagAOX
{
namespace sys
{

int runCommand( std::vector<std::string> & commandOutput, // [out] the output, line by line.  If an error, first entry contains the message.
                std::vector<std::string> & commandStderr, // [out] the output of stderr.
                std::vector<std::string> & commandList    // [in] command to be run, with one entry per command line word
              )
{
   int link[2];
   int errlink[2];

   pid_t pid;

   if (pipe(link)==-1)
   {
      commandOutput.push_back(std::string("Pipe error stdout: ") + strerror(errno));
      return -1;
   }

   if (pipe(errlink)==-1)
   {
      commandOutput.push_back(std::string("Pipe error stderr: ") + strerror(errno));
      return -1;
   }

   if ((pid = fork()) == -1)
   {
      commandOutput.push_back(std::string("Fork error: ") + strerror(errno));
      return -1;
   }

   // This is the child branch. It really runs.
   // The stdout and stderr tests pass only because it does.
   // But gcov cannot observe it.
   // On the success path execvp() replaces the process image before the
   // child's own gcov atexit hook can flush its counters.
   // On the failure path the child falls through to return -1 without exiting.
   // Forcing that from a test would let the forked child re-enter and re-run
   // the rest of this test binary. That would corrupt the run.
   // LCOV_EXCL_START
   if(pid == 0)
   {
      dup2 (link[1], STDOUT_FILENO);
      close(link[0]);
      close(link[1]);

      dup2 (errlink[1], STDERR_FILENO);
      close(errlink[0]);
      close(errlink[1]);

      std::vector<const char *>charCommandList( commandList.size()+1, NULL);
      for(int index = 0; index < (int) commandList.size(); ++index)
      {
         charCommandList[index]=commandList[index].c_str();
      }
      execvp( charCommandList[0], const_cast<char**>(charCommandList.data()));
      // Bug fix. execvp() only returns when it fails. The child used to return -1 here,
      // which made it keep running the caller's code as a second copy of the process.
      // The child must exit instead. The parent sees the closed pipes and empty output.
      _exit(127);
   }
   // LCOV_EXCL_STOP
   else
   {
      char commandOutput_c[4096];

      wait(NULL);

      close(link[1]);
      close(errlink[1]);

      int rd;
      // A read() failure here means the OS call itself failed on a pipe file
      // descriptor that this same process just created and still owns.
      // That is not reachable without corrupting the descriptor directly.
      // For example, closing it out from under the read would do that.
      // That is not a real usage pattern.
      // Bug fix. The read leaves one byte free for the terminator written below.
      // A read that filled the whole buffer used to put the terminator past the end.
      // LCOV_EXCL_START
      if ( (rd = read(link[0], commandOutput_c, sizeof(commandOutput_c) - 1)) < 0)
      {
         commandOutput.push_back(std::string("Read error: ") + strerror(errno));
         close(link[0]);
         return -1;
      }
      // LCOV_EXCL_STOP
      close(link[0]);

      std::string line;

      commandOutput_c[rd] = '\0';
      std::string commandOutputString(commandOutput_c);

      std::istringstream iss(commandOutputString);

      while (getline(iss, line))
      {
         commandOutput.push_back(line);
      }

      //----stderr
      // The same reasoning as the stdout read() above applies here.
      // Bug fix. Same as the stdout read above. One byte is left free for the terminator.
      // LCOV_EXCL_START
      if ( (rd = read(errlink[0], commandOutput_c, sizeof(commandOutput_c) - 1)) < 0)
      {
         commandStderr.push_back(std::string("Read error on stderr: ") + strerror(errno));
         close(errlink[0]);
         return -1;
      }
      // LCOV_EXCL_STOP
      close(errlink[0]);

      commandOutput_c[rd] = '\0';
      commandOutputString = commandOutput_c;

      std::istringstream iss2(commandOutputString);

      while (getline(iss2, line))
      {
         commandStderr.push_back(line);
      }

      wait(NULL);
      return 0;
   }
}





} //namespace sys
} //namespace MagAOX


