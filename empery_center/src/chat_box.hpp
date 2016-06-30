#ifndef EMPERY_CENTER_CHAT_BOX_HPP_
#define EMPERY_CENTER_CHAT_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <boost/container/flat_map.hpp>
#include <deque>
#include <vector>
#include "id_types.hpp"

namespace EmperyCenter {

class ChatMessage;
class PlayerSession;

class ChatBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const AccountUuid m_account_uuid;

	boost::container::flat_map<ChatChannelId,
		boost::container::flat_map<ChatMessageUuid, boost::shared_ptr<ChatMessage>>> m_channels;

public:
	explicit ChatBox(AccountUuid account_uuid);
	~ChatBox();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	boost::shared_ptr<ChatMessage> get(ChatMessageUuid chat_message_uuid) const;
	void get_all(std::vector<boost::shared_ptr<ChatMessage>> &ret) const;
	void get_by_channel(std::vector<boost::shared_ptr<ChatMessage>> &ret, ChatChannelId channel) const;

	void insert(const boost::shared_ptr<ChatMessage> &message);
	// void update(ChatMessageInfo info, bool throws_if_not_exists = true);
	bool remove(ChatMessageUuid chat_message_uuid) noexcept;

	void synchronize_with_player(const boost::shared_ptr<PlayerSession> &session) const;
};

inline void synchronize_chat_box_with_player(const boost::shared_ptr<const ChatBox> &chat_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	chat_box->synchronize_with_player(session);
}
inline void synchronize_chat_box_with_player(const boost::shared_ptr<ChatBox> &chat_box,
	const boost::shared_ptr<PlayerSession> &session)
{
	chat_box->synchronize_with_player(session);
}

}

#endif
