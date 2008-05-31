
#include "fluffycluster.h"
#include <sys/types.h>

class SystemSetup {
	public:	
	// Constructor - initialises environment
	SystemSetup(const ClusterInfo & ci);

	// Destructor - cleans up.
	~SystemSetup();

	private:
		const ClusterInfo &clusterinfo;
		void SetupEnv();
		void RunArpd();
		pid_t arpd_pid;

};

