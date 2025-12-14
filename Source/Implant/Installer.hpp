#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

inline bool installSystemService(const std::string& serviceName, const std::string& description) {
    // 1. Determine the executable's path
    char execPath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", execPath, sizeof(execPath) - 1);
    if (len == -1) {
        std::cerr << "Error: Could not determine executable path." << std::endl;
        return false;
    }
    execPath[len] = '\0';
    std::string executablePath(execPath);

    // 2. Generate the systemd service file content
    std::string serviceFileName = serviceName + ".service";
    std::string serviceFilePath = "/etc/systemd/system/" + serviceFileName;

    std::stringstream serviceContent;
    serviceContent << "[Unit]\n";
    serviceContent << "Description=" << description << "\n";
    serviceContent << "After=network.target\n\n";

    serviceContent << "[Service]\n";
    serviceContent << "Type=simple\n";
    serviceContent << "ExecStart=" << executablePath << " --server-mode\n"; // Pass a flag to run as a server
    serviceContent << "Restart=always\n";
    serviceContent << "User=root\n"; // You might change this to a less privileged user
    serviceContent << "\n[Install]\n";
    serviceContent << "WantedBy=multi-user.target\n";

    // 3. Write the service file
    // NOTE: This requires root/sudo privileges to write to /etc/systemd/system/
    std::ofstream outfile(serviceFilePath);
    if (!outfile.is_open()) {
        std::cerr << "Error: Could not open " << serviceFilePath << " for writing. "
                  << "This operation typically requires root/sudo privileges." << std::endl;
        return false;
    }
    outfile << serviceContent.str();
    outfile.close();

    std::cout << "Service file written to: " << serviceFilePath << std::endl;

    // 4. Execute systemd commands

    // a. Reload the systemd daemon to pick up the new unit file
    std::string reloadCmd = "systemctl daemon-reload";
    std::cout << "Executing: " << reloadCmd << std::endl;
    if (std::system(reloadCmd.c_str()) != 0) {
        std::cerr << "Error: systemctl daemon-reload failed." << std::endl;
        return false;
    }

    // b. Enable the service to start at boot
    std::string enableCmd = "systemctl enable " + serviceFileName;
    std::cout << "Executing: " << enableCmd << std::endl;
    if (std::system(enableCmd.c_str()) != 0) {
        std::cerr << "Error: systemctl enable failed." << std::endl;
        return false;
    }

    // c. Start the service immediately
    std::string startCmd = "systemctl start " + serviceFileName;
    std::cout << "Executing: " << startCmd << std::endl;
    if (std::system(startCmd.c_str()) != 0) {
        std::cerr << "Error: systemctl start failed." << std::endl;
        return false;
    }

    std::cout << "Successfully installed and started service: " << serviceName << std::endl;
    return true;
}