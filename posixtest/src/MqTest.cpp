// Copyright 2011-2021 Tyler Gilbert and Stratify Labs, Inc; see LICENSE.md

#include <chrono.hpp>
#include <test/Case.hpp>
#include <thread.hpp>
#include <var.hpp>

#include "MqTest.hpp"

MqTest::MqTest() : test::Test("posix::mq") {}

bool MqTest::execute_class_api_case() {

  {
    api::ErrorScope error_scope;
    const bool mq_exists = Mq("mq0").is_success();
    if( mq_exists ){
      Mq::unlink("mq0");
    }
    printer().key_bool("mqExists", mq_exists);
  }

  {
    api::ErrorScope error_scope;
    TEST_ASSERT(Mq("none").is_error() && error().error_number() == ENOENT);
  }

  {
    using MessageBuffer = Array<u8, 32>;
    Mq mq(Mq::Attributes().set_maximum_message_count(4).set_message_size(32),
          Mq::IsExclusive::yes, "mq0");

    {
      api::ErrorScope error_scope;
      TEST_ASSERT(Mq(Mq::Attributes().set_maximum_message_count(2).set_message_size(32),
                     Mq::IsExclusive::yes, "mq0").is_error() && error().error_number() == EEXIST);
    }

    {
      api::ErrorScope error_scope;
      var::Array<u8, 64> big_buffer;
      TEST_ASSERT(mq.send(big_buffer).is_error() && error().error_number() == EMSGSIZE);
    }

    {
      TEST_ASSERT(Mq(Mq::Attributes().set_message_count(2).set_message_size(32),
                     Mq::IsExclusive::no, "mq0").is_success());
    }

    TEST_ASSERT(is_success());

    TEST_EXPECT(mq.get_info().current_message_count() == 0);
    TEST_EXPECT(mq.get_info().maximum_message_count() == 4);

    const auto create_message = [](u8 value) {
      MessageBuffer message;
      message.fill(value);
      return message;
    };
    {
      ClockTimer clock_timer(ClockTimer::IsRunning::yes);
      {
        api::ErrorScope error_scope;
        auto receive_message = create_message(0x00);
        TEST_ASSERT(
            mq.receive_timed(receive_message, ClockTime(100_milliseconds))
                .is_error() &&
            error().error_number() == ETIMEDOUT);
      }
      const auto duration = clock_timer.stop().milliseconds();
      printer().key("receiveTimed (ms)", NumberString(duration));
      TEST_EXPECT(duration >= 100);
      TEST_EXPECT(duration < 250);
    }

    auto send_message = create_message(0xaa);
    TEST_ASSERT(mq.send(send_message).is_success());
    TEST_EXPECT(mq.get_info().current_message_count() == 1);
    TEST_ASSERT(mq.send(send_message).is_success());
    TEST_EXPECT(mq.get_info().current_message_count() == 2);
    TEST_ASSERT(mq.send(send_message).is_success());
    TEST_EXPECT(mq.get_info().current_message_count() == 3);
    TEST_ASSERT(mq.send(send_message).is_success());
    TEST_EXPECT(mq.get_info().current_message_count() == 4);

    {
      ClockTimer clock_timer(ClockTimer::IsRunning::yes);
      {
        api::ErrorScope error_scope;
        TEST_ASSERT(mq.send_timed(send_message, ClockTime(100_milliseconds))
                        .is_error() &&
                    error().error_number() == ETIMEDOUT);
      }
      const auto duration = clock_timer.stop().milliseconds();
      printer().key("sendTimed (ms)", NumberString(duration));
      TEST_EXPECT(duration >= 100);
      TEST_EXPECT(duration < 250);
    }

    auto receive_message = create_message(0x00);
    TEST_ASSERT(mq.receive(receive_message).is_success());
    TEST_EXPECT(View(send_message) == View(receive_message));
    TEST_ASSERT(mq.receive(receive_message).is_success());
    TEST_EXPECT(View(send_message) == View(receive_message));
    TEST_ASSERT(mq.receive(receive_message).is_success());
    TEST_EXPECT(View(send_message) == View(receive_message));
    TEST_ASSERT(mq.receive(receive_message).is_success());
    TEST_EXPECT(View(send_message) == View(receive_message));

    {
      TEST_ASSERT(mq.set_message_priority(1).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(2).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(3).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(4).send(send_message).is_success());

      // highest priority messages arrive first
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 4);
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 3);
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 2);
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 1);
    }

    {
      TEST_ASSERT(mq.set_message_priority(4).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(3).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(2).send(send_message).is_success());
      TEST_ASSERT(mq.set_message_priority(1).send(send_message).is_success());

      // highest priority messages arrive first
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 4 &&
                  View(receive_message) == View(send_message));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 3 &&
                  View(receive_message) == View(send_message));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 2 &&
                  View(receive_message) == View(send_message));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 1 &&
                  View(receive_message) == View(send_message));
    }

    {
      auto send_message0 = create_message(0xaa);
      auto send_message1 = create_message(0xaa);

      TEST_ASSERT(mq.set_message_priority(2).send(send_message0).is_success());
      TEST_ASSERT(mq.set_message_priority(2).send(send_message1).is_success());
      TEST_ASSERT(mq.set_message_priority(1).send(send_message0).is_success());
      TEST_ASSERT(mq.set_message_priority(1).send(send_message1).is_success());

      // oldest of the highest priority messages arrive first
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 2 &&
                  View(receive_message) == View(send_message0));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 2 &&
                  View(receive_message) == View(send_message1));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 1 &&
                  View(receive_message) == View(send_message0));
      TEST_ASSERT(mq.receive(receive_message).message_priority() == 1 &&
                  View(receive_message) == View(send_message1));
    }

    {

      auto send_buffer = create_message(0x55);
      TEST_ASSERT(mq.set_message_priority(2).send(send_buffer).is_success());
      TEST_ASSERT(mq.set_message_priority(2).send(send_buffer).is_success());
      TEST_ASSERT(mq.set_message_priority(1).send(send_buffer).is_success());
      TEST_ASSERT(mq.set_message_priority(1).send(send_buffer).is_success());

      auto thread = Thread(
          Thread::Attributes().set_joinable(),
          Thread::Construct().set_argument(&mq).set_function(
              [](void *args) -> void * {
                auto *mq = reinterpret_cast<Mq *>(args);

                MessageBuffer buffer;
                bool result = mq->receive(buffer).message_priority() == 2;
                result = mq->receive(buffer).message_priority() == 2 && result;
                result = mq->receive(buffer).message_priority() == 1 && result;
                result = mq->receive(buffer).message_priority() == 1 && result;
                if (result == false) {
                  return (void *)-1;
                }

                return nullptr;
              }));

      void *result = nullptr;

      TEST_ASSERT(thread.join(&result).is_success());
      TEST_ASSERT(result == nullptr);
    }

    //destruction is postponed until all references have closed the queue
    Mq::unlink("mq0");
    TEST_ASSERT(is_success());
  }
  TEST_ASSERT(is_success());


  return case_result();
}
