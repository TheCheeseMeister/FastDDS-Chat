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

std::vector<std::string> endThreadSignal = {};
eprosima::fastdds::dds::TypeSupport test_type(new UserChatPubSubType());

class sub_thread {
private:
    UserChatSubscriber* user_sub;
    std::string sub_topic;
    std::thread st;

public:
    sub_thread(std::string sub_topic) {
        this->sub_topic = sub_topic;
        user_sub = new UserChatSubscriber(sub_topic);
        user_sub->init();
        st = std::thread(&sub_thread::run, this);
    }

    void run() {
        user_sub->run();
    }

    UserChatSubscriber* getSub() {
        return user_sub;
    }

    std::thread* getThread() {
        return &st;
    }
};

class pub_thread {
private:
    UserChatPublisher* user_pub;
    std::string pub_topic;
    std::thread pt;

public:
    pub_thread(std::string pub_topic) {
        this->pub_topic = pub_topic;
        user_pub = new UserChatPublisher(pub_topic);
        user_pub->init();
        pt = std::thread(&pub_thread::run, this);
    }

    void run() {
        user_pub->run();
    }

    UserChatPublisher* getPub() {
        return user_pub;
    }

    std::thread* getThread() {
        return &pt;
    }
};

// View users currently added
void viewUsers(std::vector<std::string>& threaded_usernames, std::vector<pub_thread>& pubs) {
    std::cout << std::endl << "These are the users you are currently connected to:" << std::endl;

    int i = 0;
    for (std::string& user : threaded_usernames) {
        bool curr_status = pubs.at(i).getPub()->getStatus();
        std::string str = "";

        if (curr_status) str = "online";
        else str = "offline";

        std::cout << "  " + user + " (" + str + ")" << std::endl;

        i++;
    }
}

// Add new user
void addUser(std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs, std::vector<std::string>& threaded_usernames, std::string username) {
    std::string new_user = "";

    while (true) {
        std::cout << std::endl << "Enter new user: ";
        std::getline(std::cin, new_user);

        if (new_user.find(" ") != std::string::npos) {
            std::cout << "Enter a valid username." << std::endl;

            new_user = "";
        }
        else {
            break;
        }
    }

    pub_thread pub(username + "_" + new_user);
    sub_thread sub(new_user + "_" + username);

    pubs.push_back(std::move(pub));
    subs.push_back(std::move(sub));

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
void removeUser(std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs, std::vector<std::string>& threaded_usernames, std::string removed_user, std::string username) {
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

    if (pubs.at(index).getThread()->joinable()) {
        pubs.at(index).getThread()->join();
    }

    if (subs.at(index).getThread()->joinable()) {
        subs.at(index).getThread()->join();
    }

    pubs.erase(pubs.begin() + index);
    subs.erase(subs.begin() + index);

    endThreadSignal.clear();

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

void chatUser(std::string username, std::string other_user, std::vector<std::string> threaded_usernames, std::vector<pub_thread>& pubs, std::vector<sub_thread>& subs) {
    /*int index = findIndex(threaded_usernames, other_user);

    if (index == -1) {
        std::cout << "Invalid username." << std::endl;
        return;
    }

    std::vector<std::thread> curr_set = pub_subs.at(index).at(0);

    std::cout << "Chat to the user here!" << std::endl;

    while (true) {
        std::string message = "";

        std::getline(std::cin, message);

        if (message == "/exit") break;
    }*/
    int index = findIndex(threaded_usernames, other_user);

    if (index == -1) {
        std::cout << "Invalid username." << std::endl;
        return;
    }

    /*UserChatPublisher* curr_pub = pubs.at(index).getPub();
    UserChatSubscriber* curr_sub = subs.at(index);*/

    pubs.at(index).getPub()->setActive(true);
    pubs.at(index).getPub()->setActive(false);

    std::cout << "Cool, works." << std::endl;
}

int main()
{
    std::vector<pub_thread> pubs = {};
    std::vector<sub_thread> subs = {};

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
                viewUsers(threaded_usernames, pubs);
            }
            else {
                std::cout << std::endl << "You have no Users added yet." << std::endl;
            }
        }
        else if (option == 2) {
            addUser(pubs, subs, threaded_usernames, username);
        }
        else if (option == 3) {
            std::string to_chat = "";

            std::cout << std::endl << "What user would you like to chat to: ";
            std::cin >> to_chat;
            std::cin.ignore();

            chatUser(username, to_chat, threaded_usernames, pubs, subs);
        }
        else if (option == 4) {
            std::string to_remove = "";

            std::cout << std::endl << "What user would you like to remove: ";
            std::cin >> to_remove;
            std::cin.ignore();

            removeUser(pubs, subs, threaded_usernames, to_remove, username);
        }
        else if (option == 5) break;
        else {
            std::cout << std::endl << "That's not an option. Try again. (1-5)" << std::endl;
        }
    }

    // Clean up threads
    for (int i = 0; i < pubs.size(); i++) {
        std::string removed_user = threaded_usernames.at(i);

        std::string temp_pub_topic = username + "_" + removed_user;
        std::string temp_sub_topic = removed_user + "_" + username;

        endThreadSignal.push_back(temp_pub_topic);
        endThreadSignal.push_back(temp_sub_topic);

        pubs.at(i).getThread()->join();
        subs.at(i).getThread()->join();
    }

    std::cout << std::endl << "Thanks for chatting." << std::endl;
}