#include <znc/Chan.h>
#include <znc/IRCNetwork.h>

using std::map;

class CIdleRPGMod;

#define IDLERPG_JOIN_LOGIN_WAIT_TIME 10

class CIdleRPGChannel {
  public:
    CIdleRPGChannel() {}
    virtual ~CIdleRPGChannel() {}

    const CString& GetChannel() const { return m_Channel; }
    const CString& GetBotnick() const { return m_Botnick; }
    const CString& GetUsername() const { return m_Username; }
    const CString& GetPassword() const { return m_Password; }

    bool FromString(const CString& sLine) {
        m_Channel = sLine.Token(0).AsLower();
        m_Botnick = sLine.Token(1);
        m_Username = sLine.Token(2);
        m_Password = sLine.Token(3);

        return !m_Channel.empty() && !m_Botnick.empty() &&
               !m_Username.empty() && !m_Password.empty();
    }

    CString ToString() const {
        return GetChannel() + " " + GetBotnick() + " " + GetUsername() + " " +
               GetPassword();
    }

  private:
  protected:
    CString m_Channel;
    CString m_Botnick;
    CString m_Username;
    CString m_Password;
};

class CIdleRPGModTimer : public CTimer {
  public:
    CIdleRPGModTimer(CModule* pModule, CIdleRPGChannel& sChannel,
                     unsigned int uInterval, unsigned int uCycles,
                     const CString& sLabel, const CString& sDescription)
        : CTimer(pModule, uInterval, uCycles, sLabel, sDescription) {
        pChannel = sChannel;
    }

    ~CIdleRPGModTimer() override {}

  protected:
    void RunJob() override;

  private:
    CIdleRPGChannel pChannel;
};

class CIdleRPGMod : public CModule {
  public:
    MODCONSTRUCTOR(CIdleRPGMod) {
        AddHelpCommand();

        AddCommand("Add", t_d("<#channel> <botnick> <username> <password>"),
                   t_d("Adds a new channel or updates a channel"),
                   [=](const CString& sLine) { CommandAdd(sLine); });
        AddCommand("Del", t_d("<#channel>"), t_d("Removes a channel"),
                   [=](const CString& sLine) { CommandDel(sLine); });
        AddCommand("Clear", "", t_d("Clears all current settings"),
                   [=](const CString& sLine) { CommandClear(); });
        AddCommand("List", "", t_d("Displays current settings"),
                   [=](const CString& sLine) { CommandList(); });
        AddCommand("Login", t_d("[<#channel>]"),
                   t_d("Manually triggers login now (either for specific "
                       "channel or all channels)"),
                   [=](const CString& sLine) { CommandLogin(sLine); });
    }

    ~CIdleRPGMod() override {}

    bool OnLoad(const CString& sArgs, CString& sMessage) override {
        m_Channels.empty();

        // Load current channels
        for (MCString::iterator it = BeginNV(); it != EndNV(); ++it) {
            const CString& sLine = it->second;
            CIdleRPGChannel pChannel;
            auto search = m_Channels.find(pChannel.GetChannel());

            if (pChannel.FromString(sLine) && search == m_Channels.end()) {
                m_Channels[pChannel.GetChannel()] = pChannel;
            }
        }

        return true;
    }

    void CommandList() {
        if (m_Channels.empty()) {
            PutModule(t_s("No channels setup yet. Try: help add"));
            return;
        }

        CTable Table;
        Table.AddColumn(t_s("Channel"));
        Table.AddColumn(t_s("Botnick"));
        Table.AddColumn(t_s("Username"));
        Table.AddColumn(t_s("Password"));

        for (const auto& it : m_Channels) {
            Table.AddRow();
            Table.SetCell(t_s("Channel"), it.second.GetChannel());
            Table.SetCell(t_s("Botnick"), it.second.GetBotnick());
            Table.SetCell(t_s("Username"), it.second.GetUsername());
            Table.SetCell(t_s("Password"), it.second.GetPassword());
        }

        PutModule(Table);
    }

    void CommandAdd(const CString& sLine) {
        if (sLine.Token(1).empty()) {
            PutModule(
                t_s("Usage: Add <#channel> <botnick> <username> <password>"));
            return;
        }

        CString sChannel = sLine.Token(1).AsLower();
        CString sBotnick = sLine.Token(2);
        CString sUsername = sLine.Token(3);
        CString sPassword = sLine.Token(4);

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

        CIdleRPGChannel pChannel;
        pChannel.FromString(sChannel + " " + sBotnick + " " + sUsername + " " +
                            sPassword);
        m_Channels[pChannel.GetChannel()] = pChannel;
        SetNV(pChannel.GetChannel(), pChannel.ToString());

        PutModule(t_f("Saved settings for channel {1}")(pChannel.GetChannel()));
    }

    void CommandDel(const CString& sLine) {
        if (sLine.Token(1).empty()) {
            PutModule(t_s("Usage: Del <#channel>"));
            return;
        }
        CString sChannel = sLine.Token(1).AsLower();

        auto search = m_Channels.find(sChannel);

        if (search == m_Channels.end()) {
            PutModule(t_f("Channel {1} not found")(sChannel));
            return;
        }

        m_Channels.erase(search);
        DelNV(sChannel);
        PutModule(t_f("Channel {1} removed")(sChannel));
    }

    void CommandLogin(const CString& sLine) {
        CString sChannel = sLine.Token(1).AsLower();
        if (!sChannel.empty()) {
            auto search = m_Channels.find(sChannel);

            if (search == m_Channels.end()) {
                PutModule(t_f("Invalid channel {1}")(sChannel));
                return;
            }

            CIdleRPGChannel fChannel = search->second;
            Login(fChannel);
            return;
        }

        // Go through all channels and login
        for (const auto& it : m_Channels) {
            CIdleRPGChannel fChannel = it.second;
            Login(fChannel);
        }
    }

    void CommandClear() {
        ClearNV();
        m_Channels.clear();
        PutModule(t_s("All settings cleared!"));
    }

    void QueueLogin(CIdleRPGChannel& sChan) {
        RemTimer("idlerpg_login_timer_" + sChan.GetChannel());
        AddTimer(
            new CIdleRPGModTimer(this, sChan, IDLERPG_JOIN_LOGIN_WAIT_TIME, 1,
                                 "idlerpg_login_timer_" + sChan.GetChannel(),
                                 "Tries login to IdleRPG bot"));
    }

    void Login(CIdleRPGChannel& sChan) {
        // Valid channel?
        CChan* pChan = this->GetNetwork()->FindChan(sChan.GetChannel());
        if (!pChan) {
            PutModule(t_f("Error logging in: Invalid channel [{1}]")(
                sChan.GetChannel()));
            return;
        }

        // Botnick on channel?
        CNick* pBot = pChan->FindNick(sChan.GetBotnick());
        if (!pBot) {
            PutModule(
                t_f("Error logging in: Bot [{1}] not found in channel [{2}]")(
                    sChan.GetBotnick(), sChan.GetChannel()));
            return;
        }

        // Bot has op or owner?
        if (!pBot->HasPerm(CChan::Op) && !pBot->HasPerm(CChan::Owner)) {
            PutModule(t_f(
                "Error logging in: Bot [{1}] not operator in in channel [{2}]")(
                sChan.GetBotnick(), sChan.GetChannel()));
            return;
        }

        PutIRC("PRIVMSG " + sChan.GetBotnick() + " :login " +
               sChan.GetUsername() + " " + sChan.GetPassword());
        PutModule(t_s("Logging you in with " + sChan.GetBotnick() + " on " +
                      sChan.GetChannel()));
    }

    void OnJoin(const CNick& Nick, CChan& Channel) override {
        // Setup done?
        if (m_Channels.empty()) {
            return;
        }

        // Correct channel?
        auto search = m_Channels.find(Channel.GetName().AsLower());
        if (search == m_Channels.end()) {
            return;
        }

        CIdleRPGChannel fChannel = search->second;

        // Either Bot or user joins
        if (Nick.GetNick() != fChannel.GetBotnick() &&
            !GetNetwork()->GetCurNick().Equals(Nick.GetNick())) {
            return;
        }

        QueueLogin(fChannel);
    }

  private:
    map<CString, CIdleRPGChannel> m_Channels;
};

void CIdleRPGModTimer::RunJob() {
    ((CIdleRPGMod*)GetModule())->Login(pChannel);
}

template <>
void TModInfo<CIdleRPGMod>(CModInfo& Info) {
    Info.AddType(CModInfo::NetworkModule);
    Info.SetWikiPage("idlerpg");
}

NETWORKMODULEDEFS(
    CIdleRPGMod,
    t_s("Automatically handles your login to IdleRPG games/channels"))