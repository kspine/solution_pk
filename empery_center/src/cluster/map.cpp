#include "../precompiled.hpp"
#include "common.hpp"

#include <poseidon/async_job.hpp>

namespace EmperyCenter {

// FIXME  REMOVE TEST

namespace Msg {
#define MESSAGE_NAME TestMessage
#define MESSAGE_ID   101
#define MESSAGE_FIELDS \
  FIELD_STRING(str)
#include <poseidon/cbpp/message_generator.hpp>

#define MESSAGE_NAME TestNestedMessage
#define MESSAGE_ID   102
#define MESSAGE_FIELDS \
  FIELD_STRING(str)
#include <poseidon/cbpp/message_generator.hpp>
}

CLUSTER_SERVLET(Msg::TestMessage, session, req){
	LOG_EMPERY_CENTER_FATAL("Received message: ", req);
	Poseidon::enqueueAsyncJob([=]{
		for(int i = 0; i < 10; ++i){
			char str[64];
			std::sprintf(str, "nested message %d", i);
			LOG_EMPERY_CENTER_FATAL("Sending nested message: ", str);
			auto result = session->sendAndWait(Msg::TestNestedMessage(std::string(str)));
			LOG_EMPERY_CENTER_FATAL("Response: code = ", result.first, ", message = ", result.second);
		}
	});
	return std::make_pair(100, "response");
}

}
