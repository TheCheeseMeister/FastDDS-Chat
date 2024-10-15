#include "UserChatPublisher.hpp"
#include "UserChatSubscriber.hpp"

std::atomic<bool> exit_sub_thread_flag{false}; // Lets Subscriber thread know when to stop listening

void publish(UserChatPublisher* pub)
{
    pub->run();
    exit_sub_thread_flag = true;
}

void receive(UserChatSubscriber* sub)
{
    sub->run();
}

int main(int argc, char** argv) {
    std::string username = "";
    std::string other_user = ""; // Or group to listen to

    std::cout << "What's your username? ";
    std::getline(std::cin, username, '\n');

    std::cout << "Who do you want to talk to? ";
    std::getline(std::cin, other_user, '\n');

    UserChatPublisher* mypub = new UserChatPublisher(username);
    if (mypub->init())
    {
        std::cout << "Initializing subscriber..." << std::endl;
    }

    UserChatSubscriber* mysub = new UserChatSubscriber(other_user);
    if (mysub->init())
    {
        std::cout << "Starting threads..." << std::endl;
    }

    std::thread publishThread(publish, std::ref(mypub));
    std::thread receiveThread(receive, std::ref(mysub));

    publishThread.join();
    receiveThread.join();

    delete mypub;

    std::cout << "Thanks for chatting." << std::endl;

    return 0;
}