#ifndef TCGI_SES_HPP_SENTRY
#define TCGI_SES_HPP_SENTRY

#include <scriptpp/scrvar.hpp>
#include <scriptpp/confinfo.hpp>

class ScriptVector;

class SessionData {
    ScriptVariable dirname;
    ScriptVariable sess_fn;
    bool just_created, just_removed;
    ConfigInformation info;
    ScriptVariable username;
    ConfigInformation userinfo;
    bool userinfo_read, logged_in;
    class EmailData *email_data;
public:
    SessionData(const ScriptVariable &dirpath);
    ~SessionData();

    bool Validate(ScriptVariable sess_id);

    bool Create(int time_to_live);
    bool JustCreated() const { return just_created; }
    bool JustRemoved() const { return just_removed; }
    bool Renew(int time_to_live);
    bool IsValid() const;
    ScriptVariable GetId() const;   // the value for the cookie
    ScriptVariable GetId0() const;  // only the persistent part

    void Remove();

    void PerformCleanup(int time_to_live);

    void GetCurrentRoles(ScriptVector &roles) const;

    bool Login(ScriptVariable login, ScriptVariable passtoken);
    bool IsLoggedIn() const { return logged_in; }

    bool HasUser() const { return username.IsValid() && username != ""; }
    bool SetUser(ScriptVariable u);
    ScriptVariable GetUser() const { return username; }
    bool CheckUserStatus(ScriptVariable login, bool allow_pending);
    ScriptVariable GetUserStatus() const;
    ScriptVariable GetUserRealname() const;
    ScriptVariable GetUserEmail() const;
    ScriptVariable GetUserSite() const;
    bool IsChangingEmail() const;
    ScriptVariable GetUserNewEmail() const;
    int GetUserRemainingPasswords() const;
    long long GetUserLastPwdsent() const
        { return GetUserLastEvent("pwdsent"); }

    bool GenerateNewPasswords(ScriptVector &passwds, int count) const;

    bool UpdateUserLastPwdsent() { return UpdateUserLastEvent("pwdsent"); }
    bool UpdateUserLastLogin() { return UpdateUserLastEvent("login"); }
    bool UpdateUserLastSeen() { return UpdateUserLastEvent("seen"); }

    enum {
        cu_success, cu_bad_id, cu_exists, cu_bad_email, cu_email_used,
        cu_email_replaced, cu_email_recently_tried, cu_email_banned,
        cu_change_req_too_early, cu_conf_error, cu_bug
    };
    int CreateUser(ScriptVariable u, const ScriptVariable &name,
                   const ScriptVariable &email, const ScriptVariable &site,
                   ScriptVariable &confirmation_code);
    int EmailChangeRequest(const ScriptVariable &newmail,
                           ScriptVariable &confirmation_code);
    bool EmailChangeConfirm(ScriptVariable confirmation_code);
    void EmailChangeCancel();

    bool UpdateUser(const ScriptVariable &name, const ScriptVariable &site);

        // does NOT affect user/login status in any way
    bool GetProfile(ScriptVariable login, ConfigInformation &prof) const;

    void AddToPremodQueue(const ScriptVariable &pgid,
                          int cmtid, const ScriptVariable &cmt_file) const;
    void RemoveFromPremodQueue(const ScriptVariable &pgid, int cmtid);
    int GetPremodQueue(ScriptVector &res) const;
    void ForceGetQueuePosition();

private:
    ScriptVariable GetPremodQueueDir() const;

    int CheckAndReserveEmail(const ScriptVariable &email,
                             const ScriptVariable &login);

    bool EnsureUserinfoRead() const;
    ScriptVariable GetUserParameter(const char *id) const;
    long long GetUserLastEvent(const char *id) const;
    bool UpdateUserLastEvent(const char *event);

    bool Save(bool creation);
    bool SaveUserinfo();
};

#endif
