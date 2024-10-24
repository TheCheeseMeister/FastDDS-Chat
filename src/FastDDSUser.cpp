// FastDDSUser.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

// TODO: - DONE Split threaded_class into two different classes (one for publisher, one for subscriber)
//       - DONE Change threaded_objs to std::vector<std::vector<std::thread>> (pub/sub pairings will be stored in one space)
//       - Replace placeholder thread functions with publisher and subscriber functions
//       - Create history storage (std::vector<std::vector<std::string>>) that stores the history for each conversation (updates even when not tabbed in if a message is received)
//       - Implement option 3 so user can open chat with specific user and send/see received messages (/exit is now for exiting current conversations)
//       - Add storage for user logins and whatnot (I'm going to assume the pub/sub stuff will work by now)
//       - Cry about having to add customization
//       - ???
//       - Profit...?
//

#include "UserChatPublisher.hpp"
#include "UserChatSubscriber.hpp"
#include "EndThreadSignal.hpp"

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

//std::vector<std::atomic<bool>> endThreadSignals = {};
//std::vector<char> endThreadSignals = {};
//std::vector<char> endThreadSignals;
std::vector<std::string> endThreadSignal = {};
eprosima::fastdds::dds::TypeSupport test_type(new UserChatPubSubType());

class sub_thread {
public:

    void run(std::string sub_topic) {
        UserChatSubscriber* user_sub = new UserChatSubscriber(sub_topic);
        user_sub->init();
        user_sub->run();
    }
};

class pub_thread {
public:

    void run(std::string pub_topic) {
        UserChatPublisher* user_pub = new UserChatPublisher(pub_topic);
        user_pub->init();
        user_pub->run();
    }
};

// Test threaded class for figuring out pub/sub dynamics
/*class threaded_class {
private:
    std::string name;

public:
    void setName(std::string new_name) {
        this->name = new_name;
    }

    std::string getName() {
        return this->name;
    }

    void run() {

    }
};*/

// View users currently added
void viewUsers(std::vector<std::string>& threaded_usernames) {
    std::cout << std::endl << "These are the users you are currently connected to:" << std::endl;

    for (std::string& user : threaded_usernames) {
        std::cout << "  " + user << std::endl;
    }
}

// Add new user
void addUser(std::vector<std::vector<std::thread>>& threaded_objs, std::vector<std::string>& threaded_usernames, std::string username) {
    std::string new_user = "";

    while (true) {
        std::cout << std::endl << "Enter new user: ";
        std::getline(std::cin, new_user);

        //std::cin.clear();
        //std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (new_user.find(" ") != std::string::npos) {
            std::cout << "Enter a valid username." << std::endl;

            new_user = "";
        }
        else {
            break;
        }
    }

    //threaded_class test;
    pub_thread pub;
    sub_thread sub;

    std::vector<std::thread> temp_threads;
    temp_threads.emplace_back(&pub_thread::run, &pub, username + "_" + new_user);
    temp_threads.emplace_back(&sub_thread::run, &sub, new_user + "_" + username);

    threaded_objs.push_back(std::move(temp_threads));
    threaded_usernames.push_back(new_user);

    std::cout << "Successfully added " + new_user + "." << std::endl;
}

// Find index of element in vector
int findIndex(std::vector<std::string> vector, std::string search) {
    auto f = std::find(vector.begin(), vector.end(), search);

    if (f != vector.end()) {
        return f - vector.begin();
    }
    else {
        return -1;
    }
}

// Remove user
void removeUser(std::vector<std::vector<std::thread>>& threaded_objs, std::vector<std::string>& threaded_usernames, std::string removed_user, std::string username) {
    int index = findIndex(threaded_usernames, removed_user);

    if (index == -1) {
        std::cout << "Error: User was not found." << std::endl;
        return;
    }

    threaded_usernames.erase(threaded_usernames.begin() + index);

    std::string temp_pub_topic = username + "_" + removed_user;
    std::string temp_sub_topic = removed_user + "_" + username;

    endThreadSignal.push_back(temp_pub_topic);
    endThreadSignal.push_back(temp_sub_topic);

    for (std::thread& thread : threaded_objs.at(index)) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    endThreadSignal.clear();

    threaded_objs.erase(threaded_objs.begin() + index);

    std::cout << removed_user + " has been successfully removed." << std::endl;
}

// Home Menu
void printHomeMenu() {
    std::cout << std::endl << "Choose an option from below:" << std::endl;
    std::cout << "  1. View list of currently connected users." << std::endl;
    std::cout << "  2. Add a new user." << std::endl;
    std::cout << "  3. Chat with a user." << std::endl;
    std::cout << "  4. Remove a user." << std::endl;
    std::cout << "  5. Exit the program." << std::endl << std::endl;
}

// Get login info
void getCredentials(std::string& username, std::string& password) {
    std::cout << "Please enter your login info below:" << std::endl << std::endl;

    std::cout << "Username: ";
    std::cin >> username;
    std::cin.ignore();

    std::cout << "Password: ";
    std::cin >> password;
    std::cout << std::endl;
    std::cin.ignore();
}

int main()
{
    //std::vector<std::thread> threaded_objs = {};
    std::vector<std::vector<std::thread>> threaded_objs = {};
    std::vector<std::string> threaded_usernames = {};

    std::string username = "";
    std::string password = "";

    std::cout << "Fast-DDS Chat" << std::endl;
    std::cout << "----------------------------" << std::endl;

    getCredentials(username, password);

    std::cout << "----------------------------" << std::endl << std::endl;

    std::cout << "Welcome, " + username + ". Your password is " + password + "." << std::endl;

    while (true) {
        int option = -1;
        printHomeMenu();

        std::cin >> option;
        std::cin.ignore();

        if (option == 1) {
            if (!threaded_usernames.empty()) {
                viewUsers(threaded_usernames);
            }
            else {
                std::cout << std::endl << "You have no Users added yet." << std::endl;
            }
        }
        //else if (option == 2) threaded_objs.push_back(addUser(threaded_objs, threaded_usernames));
        else if (option == 2) {
            addUser(threaded_objs, threaded_usernames, username);
        }
        else if (option == 4) {
            std::string to_remove = "";

            std::cout << std::endl << "What user would you like to remove: ";
            std::cin >> to_remove;
            std::cin.ignore();

            removeUser(threaded_objs, threaded_usernames, to_remove, username);
        }
        else if (option == 5) break;
        else {
            std::cout << std::endl << "That's not an option. Try again. (1-5)" << std::endl;
        }
    }

    // Clean up threads
    int i = 0;
    for (std::vector<std::thread>& vector : threaded_objs) {
        std::string removed_user = threaded_usernames.at(i);

        std::string temp_pub_topic = username + "_" + removed_user;
        std::string temp_sub_topic = removed_user + "_" + username;

        endThreadSignal.push_back(temp_pub_topic);
        endThreadSignal.push_back(temp_sub_topic);

        for (std::thread& thread : vector) {
            thread.join();
        }
        endThreadSignal.clear();

        i++;
    }

    std::cout << std::endl << "Thanks for chatting." << std::endl;
}