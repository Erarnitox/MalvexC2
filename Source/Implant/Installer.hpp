#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <unistd.h>
#include <limits.h>

#include <Logger.hpp>

inline bool installSystemService(const std::string& serviceName, const std::string& description) {
    // 1. Determine the executable's path
    char execPath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", execPath, sizeof(execPath) - 1);
    if (len == -1) {
        logger::error("Error: Could not determine executable path.");
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
    if (not outfile.is_open()) {
        logger::warn("Error: Could not open {} for writing. This operation typically requires root/sudo privileges.", serviceFilePath);
        return false;
    }
    outfile << serviceContent.str();
    outfile.close();

    logger::info("Service file written to: {}", serviceFilePath);

    // 4. Execute systemd commands

    // a. Reload the systemd daemon to pick up the new unit file
    std::string reloadCmd = "systemctl daemon-reload";
    logger::info("Executing: {}", reloadCmd);
    if (std::system(reloadCmd.c_str()) != 0) {
        logger::error("Error: systemctl daemon-reload failed.");
        return false;
    }

    // b. Enable the service to start at boot
    std::string enableCmd = "systemctl enable " + serviceFileName;
    logger::info("Executing: {}", enableCmd);
    if (std::system(enableCmd.c_str()) != 0) {
        logger::error("Error: systemctl enable failed.");
        return false;
    }

    // c. Start the service immediately
    std::string startCmd = "systemctl start " + serviceFileName;
    logger::info("Executing: {}", startCmd);
    if (std::system(startCmd.c_str()) != 0) {
        logger::error("Error: systemctl start failed.");
        return false;
    }

    logger::info("Successfully installed and started service: {}", serviceName);
    return true;
}