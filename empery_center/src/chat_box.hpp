#ifndef EMPERY_CENTER_CHAT_BOX_HPP_
#define EMPERY_CENTER_CHAT_BOX_HPP_

#include <poseidon/cxx_util.hpp>
#include <poseidon/virtual_shared_from_this.hpp>
#include <cstddef>
#include <vector>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include "id_types.hpp"

namespace EmperyCenter {

class ChatMessage;
class PlayerSession;

class ChatBox : NONCOPYABLE, public virtual Poseidon::VirtualSharedFromThis {
private:
	const AccountUuid m_account_uuid;

	boost::shared_ptr<void> m_messages;

public:
	ChatBox(AccountUuid account_uuid);
	~ChatBox();

public:
	AccountUuid get_account_uuid() const {
		return m_account_uuid;
	}

	boost::shared_ptr<ChatMessage> get(ChatMessageUuid chat_message_uuid) const;
	void get_all(std::vector<boost::shared_ptr<ChatMessage>> &ret) const;

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
