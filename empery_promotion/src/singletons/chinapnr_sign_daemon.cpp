#include "../precompiled.hpp"
#include "chinapnr_sign_daemon.hpp"
#include <poseidon/singletons/job_dispatcher.hpp>
#include <poseidon/job_promise.hpp>
#include <poseidon/ip_port.hpp>
#include <poseidon/tcp_client_base.hpp>

namespace EmperyPromotion {

namespace {
	class SignClient : public Poseidon::TcpClientBase {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

		int m_errCode;
		std::string m_result;

		std::size_t m_payloadLen;
		Poseidon::StreamBuffer m_payload;

	public:
		SignClient(unsigned port, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(Poseidon::IpPort(sslit("127.0.0.1"), port), false)
			, m_promise(std::move(promise))
			, m_errCode(-1), m_result()
			, m_payloadLen((std::size_t)-1)
		{
		}

	protected:
		void onClose(int errCode) noexcept override {
			LOG_EMPERY_PROMOTION_DEBUG("Connection closed: errCode = ", errCode);

			m_promise->setSuccess();

			Poseidon::TcpClientBase::onClose(errCode);
		}

		void onReadAvail(Poseidon::StreamBuffer data) override {
			PROFILE_ME;

			m_payload.splice(data);

			char str[256];

			if(m_payloadLen == (std::size_t)-1){
				if(m_payload.size() < 4){
					return;
				}
				str[m_payload.peek(str, 4)] = 0;
				char *end;
				const auto payloadLen = std::strtoul(str, &end, 10);
				if(*end){
					LOG_EMPERY_PROMOTION_ERROR("Invalid payload length from NPC server: ", str);
					DEBUG_THROW(Exception, sslit("Invalid payload length from NPC server"));
				}
				if(m_payload.size() - 4 < payloadLen){
					return;
				}
				LOG_EMPERY_PROMOTION_DEBUG("Payload from NPC server: ", m_payload.dump());
				m_payloadLen = payloadLen;
			}
			m_payload.discard(11);

			str[m_payload.get(str, 4)] = 0;
			char *end;
			const auto errCode = std::strtol(str, &end, 10);
			if(*end){
				LOG_EMPERY_PROMOTION_ERROR("Invalid error code from NPC server: ", str);
				DEBUG_THROW(Exception, sslit("Invalid error code from NPC server"));
			}
			m_errCode = errCode;

			if(errCode == 0){
				std::string str;
				str.resize(256);
				m_payload.get(&str[0], str.size());
				m_result.swap(str);
			}

			shutdownRead();
			shutdownWrite();
		}

	public:
		const std::string &getResult() const {
			if(m_errCode != 0){
				LOG_EMPERY_PROMOTION_ERROR("Error returned from NPC server: errCode = ", m_errCode);
				DEBUG_THROW(Exception, sslit("Error returned from NPC server"));
			}
			return m_result;
		}
	};

	class ValidateClient : public Poseidon::TcpClientBase {
	private:
		const boost::shared_ptr<Poseidon::JobPromise> m_promise;

		int m_errCode;
		bool m_result;

		std::size_t m_payloadLen;
		Poseidon::StreamBuffer m_payload;

	public:
		ValidateClient(unsigned port, boost::shared_ptr<Poseidon::JobPromise> promise)
			: Poseidon::TcpClientBase(Poseidon::IpPort(sslit("127.0.0.1"), port), false)
			, m_promise(std::move(promise))
			, m_errCode(-1), m_result(false)
			, m_payloadLen((std::size_t)-1)
		{
		}

	protected:
		void onClose(int errCode) noexcept override {
			LOG_EMPERY_PROMOTION_DEBUG("Connection closed: errCode = ", errCode);

			m_promise->setSuccess();

			Poseidon::TcpClientBase::onClose(errCode);
		}

		void onReadAvail(Poseidon::StreamBuffer data) override {
			PROFILE_ME;

			m_payload.splice(data);

			char str[256];

			if(m_payloadLen == (std::size_t)-1){
				if(m_payload.size() < 4){
					return;
				}
				str[m_payload.peek(str, 4)] = 0;
				char *end;
				const auto payloadLen = std::strtoul(str, &end, 10);
				if(*end){
					LOG_EMPERY_PROMOTION_ERROR("Invalid payload length from NPC server: ", str);
					DEBUG_THROW(Exception, sslit("Invalid payload length from NPC server"));
				}
				if(m_payload.size() - 4 < payloadLen){
					return;
				}
				LOG_EMPERY_PROMOTION_DEBUG("Payload from NPC server: ", m_payload.dump());
				m_payloadLen = payloadLen;
			}
			m_payload.discard(11);

			str[m_payload.get(str, 4)] = 0;
			char *end;
			const auto errCode = std::strtol(str, &end, 10);
			if(*end){
				LOG_EMPERY_PROMOTION_ERROR("Invalid error code from NPC server: ", str);
				DEBUG_THROW(Exception, sslit("Invalid error code from NPC server"));
			}
			m_errCode = errCode;

			if(errCode == 0){
				m_result = true;
			}

			shutdownRead();
			shutdownWrite();
		}

	public:
		bool getResult() const {
			if(m_errCode != 0){
				LOG_EMPERY_PROMOTION_ERROR("Error returned from NPC server: errCode = ", m_errCode);
				DEBUG_THROW(Exception, sslit("Error returned from NPC server"));
			}
			return m_result;
		}
	};
}

std::string ChinaPnrSignDaemon::sign(const std::string &merId, const std::string &serial, boost::uint64_t /* createdTime */,
	boost::uint64_t amount, const std::string &retUrl, const std::string &usrMp, const std::string &bgRetUrl)
{
	PROFILE_ME;

	char str[256];
	unsigned len;

	std::string dataToCheck;
	dataToCheck.reserve(1023);
	dataToCheck.append("10", 2);  // Version
	dataToCheck.append("Buy", 3); // CmdId
	dataToCheck.append(merId);    // MerId
	dataToCheck.append(serial);   // OrdId
	len = (unsigned)std::sprintf(str, "%llu.%02u", (unsigned long long)(amount / 100), (unsigned)(amount % 100));
	dataToCheck.append(str, len); // OrdAmt
	dataToCheck.append("RMB", 3); // CurCode
	dataToCheck.append(retUrl);   // RetUrl
	dataToCheck.append(usrMp);    // UsrMp
	dataToCheck.append(bgRetUrl); // BgRetUrl

	Poseidon::StreamBuffer payload;
	payload.put('S');
	payload.put(merId);
	len = (unsigned)std::sprintf(str, "%04zu", dataToCheck.size());
	payload.put(str, len);
	payload.put(dataToCheck);

	Poseidon::StreamBuffer dataToSend;
	len = (unsigned)std::sprintf(str, "%04zu", payload.size());
	dataToSend.put(str, len);
	dataToSend.splice(payload);
	LOG_EMPERY_PROMOTION_DEBUG("Prepared data to sign: ", dataToSend.dump());

	const auto promise = boost::make_shared<Poseidon::JobPromise>();

	const auto port = getConfig<unsigned>("chinapnr_npc_port", 8733);
	const auto client = boost::make_shared<SignClient>(port, promise);
	client->send(std::move(dataToSend));
	client->goResident();

	Poseidon::JobDispatcher::yield(promise);
	promise->checkAndRethrow();
	return client->getResult();
}
bool ChinaPnrSignDaemon::check(const std::string &cmdId, const std::string &merId, const std::string &respCode,
	const std::string &trxId, const std::string &ordAmt, const std::string &curCode, const std::string &pid,
	const std::string &ordId, const std::string &merPriv, const std::string &retType, const std::string &divDetails,
	const std::string &gateId, const std::string &chkValue)
{
	PROFILE_ME;

	char str[256];
	unsigned len;

	std::string dataToCheck;
	dataToCheck.reserve(1023);
	dataToCheck.append(cmdId);
	dataToCheck.append(merId);
	dataToCheck.append(respCode);
	dataToCheck.append(trxId);
	dataToCheck.append(ordAmt);
	dataToCheck.append(curCode);
	dataToCheck.append(pid);
	dataToCheck.append(ordId);
	dataToCheck.append(merPriv);
	dataToCheck.append(retType);
	dataToCheck.append(divDetails);
	dataToCheck.append(gateId);

	Poseidon::StreamBuffer payload;
	payload.put('V');
	payload.put(merId);
	len = (unsigned)std::sprintf(str, "%04zu", dataToCheck.size());
	payload.put(str, len);
	payload.put(dataToCheck);
	payload.put(chkValue);

	Poseidon::StreamBuffer dataToSend;
	len = (unsigned)std::sprintf(str, "%04zu", payload.size());
	dataToSend.put(str, len);
	dataToSend.splice(payload);
	LOG_EMPERY_PROMOTION_DEBUG("Prepared data to verify: ", dataToSend.dump());

	const auto promise = boost::make_shared<Poseidon::JobPromise>();
	const auto port = getConfig<unsigned>("chinapnr_npc_port", 8733);

	const auto client = boost::make_shared<ValidateClient>(port, promise);
	client->send(std::move(dataToSend));
	client->goResident();

	Poseidon::JobDispatcher::yield(promise);
	promise->checkAndRethrow();
	return client->getResult();
}

}
