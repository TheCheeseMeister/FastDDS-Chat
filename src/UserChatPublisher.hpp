/**
 * @file UserChatPublisher.hpp
 */

#include "UserChatPubSubTypes.hpp";
//#include "Globals.hpp"
#include <chrono>
#include <thread>
#include <string>
#include <atomic>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

class UserChatPublisher {
private:
    UserChat user_message_;
    DomainParticipant* participant_;
    Publisher* publisher_;
    Topic* topic_;
    DataWriter* writer_;
    TypeSupport type_;

    std::atomic<bool> active;           // Whether Publisher is accepting input
    std::atomic<bool> status;           // Whether Publisher is online or not (matched with subscriber)
    std::vector<std::string>* history;  // Ongoing history of chat
    std::vector<std::string>* end_signal; // Tells thread to end

    std::string username;
    std::string topic_name;

    class PubListener : public DataWriterListener
    {
    private:
        UserChatPublisher* publisher_;
    public:
        PubListener(UserChatPublisher* publisher) : matched_(0), publisher_(publisher) {}
        ~PubListener() override {}

        void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override {
            if (info.current_count_change == 1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher matched." << std::endl;
                publisher_->setStatus(true);
            }
            else if (info.current_count_change == -1)
            {
                matched_ = info.total_count;
                std::cout << "Publisher unmatched." << std::endl;
                publisher_->setStatus(false);
            }
            else {
                std::cout << info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
            }
        }

        std::atomic_int matched_;
    } listener_;

public:
    UserChatPublisher(std::string topic_name, std::string name, std::vector<std::string>* curr_history, std::vector<std::string>* signal)
        : participant_(nullptr)
        , publisher_(nullptr)
        , topic_(nullptr)
        , writer_(nullptr)
        , type_(new UserChatPubSubType())
        , listener_(this)
        , history(curr_history)
        , end_signal(signal)
    {
        this->topic_name = topic_name;
        this->active = false;
        this->status = false;
        this->username = name;
    }

    virtual ~UserChatPublisher() {
        if (writer_ != nullptr)
        {
            publisher_->delete_datawriter(writer_);
        }
        if (publisher_ != nullptr)
        {
            participant_->delete_publisher(publisher_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init()
    {
        user_message_.index(0);
        user_message_.username(username);

        DomainParticipantQos participantQos;
        participantQos.name("Participant_publisher");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr)
        {
            return false;
        }

        type_.register_type(participant_);

        // Creates topic named after username to publish from
        topic_ = participant_->create_topic(topic_name, "UserChat", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr)
        {
            return false;
        }

        publisher_ = participant_->create_publisher(PUBLISHER_QOS_DEFAULT, nullptr);

        if (publisher_ == nullptr)
        {
            return false;
        }

        writer_ = publisher_->create_datawriter(topic_, DATAWRITER_QOS_DEFAULT, &listener_);

        if (writer_ == nullptr)
        {
            return false;
        }
        return true;
    }

    bool publish()
    {
        if (listener_.matched_ > 0)
        {
            user_message_.index(user_message_.index() + 1);
            writer_->write(&user_message_);
            return true;
        }
        return false;
    }

    // Whether to get input from user or not
    void setActive(bool set) {
        active.store(set);
    }

    bool getActive() {
        return active.load();
    }

    // Signals online or offline
    void setStatus(bool set) {
        status.store(set);
    }

    bool getStatus() {
        return status.load();
    }

    void run()
    {
        uint32_t samples_sent = 0;
        while (true)
        {
            //if (std::find(endThreadSignal.begin(), endThreadSignal.end(), topic_name) != endThreadSignal.end()) break;
            if (std::find(end_signal->begin(), end_signal->end(), topic_name) != end_signal->end()) break;

            if (getActive()) {
                if (!getStatus()) {
                    std::cout << std::endl << "Other user is offline now. Last message discarded. Press any key to go back to main ui...";
                    getchar();

                    setActive(false);
                }
                else if (publish())
                {
                    std::string message = "";
                    std::string exit = "/exit";

                    std::getline(std::cin, message, '\n');

                    if (message == exit) {
                        setActive(false);

                        message = "";
                    }
                    else if (getStatus()){
                        user_message_.message(message);

                        std::string str = user_message_.username() + ": " + message;
                        history->push_back(str);
                    }
                }
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
};