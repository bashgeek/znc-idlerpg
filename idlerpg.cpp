#include <znc/IRCNetwork.h>
#include <znc/Chan.h>

#define IDLERPG_JOIN_LOGIN_WAIT_TIME 10

class CIdleRPGMod;

class CIdleRPGModTimer : public CTimer {
  public:
    CIdleRPGModTimer(CModule* pModule, unsigned int uInterval,
                   unsigned int uCycles, const CString& sLabel,
                   const CString& sDescription)
        : CTimer(pModule, uInterval, uCycles, sLabel, sDescription) {}

    ~CIdleRPGModTimer() override {}

  protected:
    void RunJob() override;
};

class CIdleRPGMod : public CModule {
	void Set (const CString& sCommand) {
		if (sCommand.Token(1).empty()) {
			PutModule(t_s("Usage: <#channel> <botnick> <username> <password>"));
			return;
		}

		CString sChannel = sCommand.Token(1);
		CString sBotnick = sCommand.Token(2);
		CString sUsername = sCommand.Token(3);
		CString sPassword = sCommand.Token(4);

		if (sChannel.empty()) {
			PutModule("Channel not supplied");
			return;
		}
		if (sBotnick.empty()) {
			PutModule("Botnick not supplied");
			return;
		}
		if (sUsername.empty()) {
			PutModule("Username not supplied");
			return;
		}
		if (sPassword.empty()) {
			PutModule("Password not supplied");
			return;
		}

		m_channel = sChannel;
		m_botnick = sBotnick;
		m_username = sUsername;
		m_password = sPassword;
		Save();

		PutModule(t_s("Settings saved!"));
	}

	void Get (const CString& sCommand) {
		if (m_channel.empty()) {
			PutModule(t_s("No settings found."));
			return;
		}

		PutModule(t_s("Current settings are as following:"));
		PutModule(t_f("Channel: {1} - Botnick: {2} - Username: {3} - Password: {4}")(m_channel, m_botnick, m_username, m_password));
	}

	void Clear (const CString& sCommand) {
		m_channel = "";
		m_botnick = "";
		m_username = "";
		m_password = "";
		Save();

		PutModule(t_s("All settings cleared."));
	}

	void LoginCommand (const CString& sCommand) {
		if (m_channel.empty()) {
			PutModule(t_s("You need to configure this module first. Try: help set"));
			return;
		}

		QueueLogin(0);
	}


	public:
		MODCONSTRUCTOR(CIdleRPGMod) {
			AddHelpCommand();

			AddCommand("Set", t_d("<#channel> <botnick> <username> <password>"), t_d("Sets all the required information to function."), [=](const CString& sLine) { Set(sLine); });
			AddCommand("Get", "", t_d("Displays current saved settings"), [=](const CString& sLine) { Get(sLine); });
			AddCommand("Clear", "", t_d("Resets all settings"), [=](const CString& sLine) { Clear(sLine); });
			AddCommand("Login", "", t_d("Manually triggers login now"), [=](const CString& sLine) { LoginCommand(sLine); });
		}

		~CIdleRPGMod() override {}

		bool OnLoad(const CString& sArgs, CString& sMessage) override {
			m_channel = GetNV("channel");
			m_botnick = GetNV("botnick");
			m_username = GetNV("username");
			m_password = GetNV("password");

			return true;
		}

		void OnJoin(const CNick& Nick, CChan& Channel) override {
			// Setup done?
			if (m_channel.empty()) {
				return;
			}

			// Correct channel?
			if (Channel.GetName().AsLower() != m_channel.AsLower()) {
				return;
			}

			// Either Bot or user joins
			if (Nick.GetNick() != m_botnick && !GetNetwork()->GetCurNick().Equals(Nick.GetNick())) {
				return;
			}

			QueueLogin(IDLERPG_JOIN_LOGIN_WAIT_TIME);
		}

		void Login () {
			// Valid channel?
			CChan* pChan = this->GetNetwork()->FindChan(m_channel);
			if (!pChan) {
				PutModule(t_f("Error logging in: Invalid channel [{1}]")(m_channel));
				return;
			}

			// Botnick on channel?
			CNick* pBot = pChan->FindNick(m_botnick);
			if (!pBot) {
				PutModule(t_f("Error logging in: Bot [{1}] not found in channel [{2}]")(m_botnick, m_channel));
				return;
			}

			// Bot has op?
			if (!pBot->HasPerm(CChan::Op)) {
				PutModule(t_f("Error logging in: Bot [{1}] not operator in in channel [{2}]")(m_botnick, m_channel));
				return;
			}

			PutIRC("PRIVMSG " + m_botnick + " :login " + m_username + " " + m_password);
			PutModule(t_s("Logging you in..."));
		}

	private:
		CString m_channel;
		CString m_botnick;
		CString m_username;
		CString m_password;

		void QueueLogin(int seconds) {
			if (m_channel.empty()) {
				return;
			}

			RemTimer("idlerpg_login_timer");
      AddTimer(new CIdleRPGModTimer(this, seconds, 1, "idlerpg_login_timer", "Tries login to IdleRPG bot"));
		}
		void Save() {
			SetNV("channel", m_channel);
			SetNV("botnick", m_botnick);
			SetNV("username", m_username);
			SetNV("password", m_password);
		}
};

void CIdleRPGModTimer::RunJob() { ((CIdleRPGMod*)GetModule())->Login(); }

template <>
void TModInfo<CIdleRPGMod>(CModInfo& Info) {
	Info.AddType(CModInfo::NetworkModule);
	Info.SetWikiPage("idlerpg");
}

NETWORKMODULEDEFS(
	CIdleRPGMod,
	t_s("Automatically handles your login to an IdleRPG game/channel")
)