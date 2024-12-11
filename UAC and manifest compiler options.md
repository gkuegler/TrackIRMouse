An explanation of UAC on windows.

Correct settings:
<requestedExecutionLevel level="requireAdministrator" uiAccess="false" />
uiAccess can be true only when:
    The app is digitally signed.
    Must be in a secure location in the file system LINK
What you can do is:
    Check if your application really needs uiAccess LINK
    Sign the application

# Local Policy
Set the following policies to enable UAC for my unsigned app:
- Conmputer Configuration/Windows Settings/Security Settings/Local Policies/Security Options/User Account Control: Only elevate executable files that are signed and validated = "Disabled"
- Conmputer Configuration/Windows Settings/Security Settings/Local Policies/Security Options/User Account Control: Only elevate UIAccess applications that are installed in secure locations = "Disabled"