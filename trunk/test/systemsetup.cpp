
#include "systemsetup.h"
#include "utils.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <stdexcept>

#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

using namespace std;

static int RunSubprocess(const char *cmd)
{
	// Fork subprocess...
	pid_t pid = fork();
	if (pid == -1) {
		throw std::runtime_error("Fork failed");
	}
	if (pid == 0) {
		// In child proc.
		execl(cmd, cmd, (const char *) 0); // Null terminate arg list
		// If we get this far, it's bad.
		// We can't do anything else because various
		// resources are shared with parent.
		// Don't call exit() because unflushed buffers could
		// get flushed etc, creating unexpected behaviour.
		_exit(1);
	}
	// In parent...
	// Wait for pid to terminate...
	int status=0;
	waitpid(pid, &status, 0 /* options */ );
	return status;
}

void SystemSetup::RunArpd()
{
	// Fork a child pid
	arpd_pid = fork();
	if (arpd_pid == -1) {
		throw std::runtime_error("Arpd Fork failed");
	}
	if (arpd_pid == 0) {
		// Child...
		// execl the arp daemon.
		execl("arpd","arpd",
			clusterinfo.ifname,
			toString(clusterinfo.ipaddress).c_str(),
			toString(clusterinfo.macaddress).c_str(),
			(const char *) 0);
		// If we get this far, it's all gone wrong.
		_exit(1);
	}
	// Parent process. Good.
}

void SystemSetup::SetupEnv()
{
	// Set path appropriately.
	setenv("PATH",".", 1); // fixme.
	// Set other env vars
	setenv("INTERFACE", clusterinfo.ifname,1 );	
	setenv("IP", clusterinfo.ipaddress.toString().c_str(),1);	
	setenv("CLUSTERMAC", toString(clusterinfo.macaddress).c_str(),1);
	setenv("NFQUEUE_NUM","42",1);
	// NETMASK - not set because we don't really know it.
}

SystemSetup::SystemSetup(const ClusterInfo & ci) : clusterinfo(ci)
{
	arpd_pid=0;
	SetupEnv();
	// Set up IP tables via fc-init.sh
	int res = RunSubprocess("fc-init.sh");
	if (res != 0) {
		throw std::runtime_error("Subprocess returned " + toString(res));
	}
	// Kick off arp daemon
	RunArpd();
}

SystemSetup::~SystemSetup()
{
	// Kill off arp daemon
	kill(arpd_pid, SIGTERM); // May fail, we ignore.
	// Wait for arpd to quit...
	int status=0;
	waitpid(arpd_pid, &status, 0 /* options */ );
	if (status != 0) {
		std::cerr << "Arpd returned nonzero exit code " << status << std::endl;
	}
	
	SetupEnv();
	// Clean up IP tables via fc-finish.sh
	int res = RunSubprocess("fc-finish.sh");
	if (res != 0) {
		throw std::runtime_error("Subprocess returned " + toString(res));
	}
}

