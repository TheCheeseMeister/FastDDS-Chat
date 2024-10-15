/**
 * @file UserChatPublisher.hpp
 */

#include "UserChatPubSubTypes.hpp";
#include <chrono>
#include <thread>
#include <string>

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

        std::string topic_name;

        class PubListener : public DataWriterListener
        {
            public:
                PubListener() : matched_(0) {}
                ~PubListener() override {}

                void on_publication_matched(DataWriter*, const PublicationMatchedStatus& info) override {
                    if (info.current_count_change == 1)
                    {
                        matched_ = info.total_count;
                        std::cout << "Publisher matched." << std::endl;
                    }
                    else if (info.current_count_change == -1)
                    {
                        matched_ = info.total_count;
                        std::cout << "Publisher unmatched." << std::endl;
                    }
                    else{
                        std::cout << info.current_count_change << " is not a valid value for PublicationMatchedStatus current count change." << std::endl;
                    }
                }

                std::atomic_int matched_;
        } listener_;

    public:
        UserChatPublisher(std::string topic_name)
            : participant_(nullptr)
            , publisher_(nullptr)
            , topic_(nullptr)
            , writer_(nullptr)
            , type_(new UserChatPubSubType())
        {
            this->topic_name = topic_name;
        }

        virtual ~UserChatPublisher(){
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
            user_message_.username(topic_name);

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

        void run()
        {
            uint32_t samples_sent = 0;
            while(true)
            {
                if (publish())
                {
                    std::string message = "";
                    std::string exit = "/exit";
                    std::getline(std::cin, message, '\n');

                    // quit on /exit
                    if (message == exit) {
                        break;
                    }

                    user_message_.message(message);

                    samples_sent++;
                    //std::cout << user_message_.username() << ": " << user_message_.message() << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
};