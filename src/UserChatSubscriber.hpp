/**
 * @file UserChatSubscriber.hpp
 */

#include "UserChatPubSubTypes.hpp";
#include "Globals.hpp"
#include <chrono>
#include <thread>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/subscriber/DataReader.hpp>
#include <fastdds/dds/subscriber/DataReaderListener.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/SampleInfo.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>
#include <fastdds/dds/topic/TypeSupport.hpp>

using namespace eprosima::fastdds::dds;

class UserChatSubscriber
{
private:
    DomainParticipant* participant_;
    Subscriber* subscriber_;
    DataReader* reader_;
    Topic* topic_;
    TypeSupport type_;

    std::string topic_name;
    std::vector<std::string>* history;  // Ongoing history of chat

    class SubListener : public DataReaderListener
    {
    private:
        UserChatSubscriber* subscriber_;
        std::string last_received_message;
    public:
        SubListener(UserChatSubscriber* subscriber) : samples_(0), subscriber_(subscriber) { last_received_message = ""; }
        ~SubListener() override {}

        void on_subscription_matched(DataReader*, const SubscriptionMatchedStatus& info) override
        {
            if (info.current_count_change == 1) {
                //std::cout << "Subscriber matched." << std::endl;
            }
            else if (info.current_count_change == -1) {
                //std::cout << "Subscriber unmatched." << std::endl;
            }
            else {
                std::cout << info.current_count_change << " is not a valid value for SubscriptionMatchedStatus current count change." << std::endl;
            }
        }

        void on_data_available(DataReader* reader) override
        {
            SampleInfo info;
            if (reader->take_next_sample(&user_message_, &info) == eprosima::fastdds::dds::RETCODE_OK) {
                if (info.valid_data)
                {
                    samples_++;

                    if (user_message_.username() != "" && user_message_.message() != "") {
                        std::string str = user_message_.username() + ": " + user_message_.message();
                        std::vector<std::string>* curr_history = subscriber_->getHistory();

                        if (last_received_message == "") {
                            if (curr_chat_tab.at(0) == "in" && curr_chat_tab.at(1) == subscriber_->getTopicName()) {
                                std::cout << user_message_.username() + ": ";
                                std::cout << user_message_.message() << std::endl;
                            }

                            curr_history->push_back(str);
                            last_received_message = str;
                        }
                        else if (last_received_message != str) {
                            if (curr_chat_tab.at(0) == "in" && curr_chat_tab.at(1) == subscriber_->getTopicName()) {
                                std::cout << user_message_.username() + ": ";
                                std::cout << user_message_.message() << std::endl;
                            }

                            curr_history->push_back(str);
                            last_received_message = str;
                        }
                    }
                }
            }
        }

        UserChat user_message_;

        std::atomic_int samples_;
    }
    listener_;

public:
    UserChatSubscriber(std::string topic_name, std::vector<std::string>* curr_history)
        : participant_(nullptr)
        , subscriber_(nullptr)
        , topic_(nullptr)
        , reader_(nullptr)
        , type_(new UserChatPubSubType())
        , listener_(this)
        , history(curr_history)
    {
        this->topic_name = topic_name;
    }

    virtual ~UserChatSubscriber()
    {
        if (reader_ != nullptr)
        {
            subscriber_->delete_datareader(reader_);
        }
        if (topic_ != nullptr)
        {
            participant_->delete_topic(topic_);
        }
        if (subscriber_ != nullptr)
        {
            participant_->delete_subscriber(subscriber_);
        }
        DomainParticipantFactory::get_instance()->delete_participant(participant_);
    }

    bool init()
    {
        DomainParticipantQos participantQos;
        participantQos.name("Participant_subscriber");

        participant_ = DomainParticipantFactory::get_instance()->create_participant(0, participantQos);

        if (participant_ == nullptr) {
            return false;
        }

        type_.register_type(participant_);

        // Creates topic named after the user/group Subscriber will look for
        topic_ = participant_->create_topic(topic_name, "UserChat", TOPIC_QOS_DEFAULT);

        if (topic_ == nullptr) {
            return false;
        }

        subscriber_ = participant_->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

        if (subscriber_ == nullptr)
        {
            return false;
        }

        reader_ = subscriber_->create_datareader(topic_, DATAREADER_QOS_DEFAULT, &listener_);

        if (reader_ == nullptr)
        {
            return false;
        }

        return true;
    }

    std::string getTopicName() {
        return topic_name;
    }

    std::vector<std::string>* getHistory() {
        return history;
    }

    void run() {
        while (true) {
            if (std::find(endThreadSignal.begin(), endThreadSignal.end(), topic_name) != endThreadSignal.end()) break;

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
};