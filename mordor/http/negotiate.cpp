// Copyright (c) 2009 - Mozy, Inc.

#include "negotiate.h"

#include "http.h"
#include "mordor/string.h"

#pragma comment(lib, "secur32.lib")

namespace Mordor {
namespace HTTP {

NegotiateAuth::NegotiateAuth(const std::string &username,
                             const std::string &password)
    : m_username(toUtf16(username)),
      m_password(toUtf16(password))
{
    SecInvalidateHandle(&m_creds);
    SecInvalidateHandle(&m_secCtx);
    size_t pos = m_username.find(L'\\');
    if (pos != std::wstring::npos) {
        m_domain = m_username.substr(0, pos);
        m_username = m_username.substr(pos + 1);
    }
}

NegotiateAuth::~NegotiateAuth()
{
    if (SecIsValidHandle(&m_creds)) {
        FreeCredentialHandle(&m_creds);
        SecInvalidateHandle(&m_creds);
    }
    if (SecIsValidHandle(&m_secCtx)) {
        FreeCredentialHandle(&m_secCtx);
        SecInvalidateHandle(&m_secCtx);
    }
}

bool
NegotiateAuth::authorize(const AuthParams &challenge, AuthParams &authorization,
    const URI &uri)
{
    SECURITY_STATUS status;
    std::wstring packageW = toUtf16(challenge.scheme);
    std::string param = challenge.base64;

    std::string outboundBuffer;
    SecBufferDesc outboundBufferDesc;
    SecBuffer outboundSecBuffer;
    TimeStamp lifetime;
    ULONG contextAttributes;

    outboundBuffer.resize(4096);
    outboundBufferDesc.ulVersion = 0;
    outboundBufferDesc.cBuffers = 1;
    outboundBufferDesc.pBuffers = &outboundSecBuffer;
    outboundSecBuffer.BufferType = SECBUFFER_TOKEN;
    outboundSecBuffer.pvBuffer = &outboundBuffer[0];
    outboundSecBuffer.cbBuffer = (unsigned long)outboundBuffer.size();

    if (param.empty()) {
        // No response from server; we're starting a new session
        if (SecIsValidHandle(&m_creds))
            return false;

        SEC_WINNT_AUTH_IDENTITY_W id;
        id.User = (unsigned short *)m_username.c_str();
        id.UserLength = (unsigned long)m_username.size();
        id.Domain = (unsigned short *)m_domain.c_str();
        id.DomainLength = (unsigned long)m_domain.size();
        id.Password = (unsigned short *)m_password.c_str();
        id.PasswordLength = (unsigned long)m_password.size();
        id.Flags = SEC_WINNT_AUTH_IDENTITY_UNICODE;
        status = AcquireCredentialsHandleW(NULL,
            (wchar_t *)packageW.c_str(),
            SECPKG_CRED_OUTBOUND,
            NULL,
            m_username.empty() ? NULL : &id,
            NULL,
            NULL,
            &m_creds,
            &lifetime);
        if (!SUCCEEDED(status))
            MORDOR_THROW_EXCEPTION_FROM_ERROR_API(status, "AcquireCredentialsHandleW");

        status = InitializeSecurityContextW(
            &m_creds,
            NULL,
            (wchar_t *)toUtf16(uri.toString()).c_str(),
            ISC_REQ_CONFIDENTIALITY,
            0,
            SECURITY_NATIVE_DREP,
            NULL,
            0,
            &m_secCtx,
            &outboundBufferDesc,
            &contextAttributes,
            &lifetime);
    } else {
        // Prepare the response from the server
        std::string inboundBuffer = base64decode(param);
        SecBufferDesc inboundBufferDesc;
        SecBuffer inboundSecBuffer;

        inboundBufferDesc.ulVersion = 0;
        inboundBufferDesc.cBuffers = 1;
        inboundBufferDesc.pBuffers = &inboundSecBuffer;
        inboundSecBuffer.BufferType = SECBUFFER_TOKEN;
        inboundSecBuffer.pvBuffer = &inboundBuffer[0];
        inboundSecBuffer.cbBuffer = (unsigned long)inboundBuffer.size();

        status = InitializeSecurityContextW(
            &m_creds,
            &m_secCtx,
            (wchar_t *)toUtf16(uri.toString()).c_str(),
            ISC_REQ_CONFIDENTIALITY,
            0,
            SECURITY_NATIVE_DREP,
            &inboundBufferDesc,
            0,
            &m_secCtx,
            &outboundBufferDesc,
            &contextAttributes,
            &lifetime);
    }

    if (status == SEC_I_COMPLETE_NEEDED ||
        status == SEC_I_COMPLETE_AND_CONTINUE) {
        status = CompleteAuthToken(&m_secCtx, &outboundBufferDesc);
    }

    if (!SUCCEEDED(status))
        MORDOR_THROW_EXCEPTION_FROM_ERROR(status);

    outboundBuffer.resize(outboundSecBuffer.cbBuffer);
    authorization.scheme = challenge.scheme;
    authorization.base64 = base64encode(outboundBuffer);
    authorization.parameters.clear();
    return true;
}

bool
NegotiateAuth::authorize(const Response &challenge, Request &nextRequest)
{
    MORDOR_ASSERT(challenge.status.status == UNAUTHORIZED ||
        challenge.status.status == PROXY_AUTHENTICATION_REQUIRED);
    bool proxy = challenge.status.status == PROXY_AUTHENTICATION_REQUIRED;
    const ChallengeList &authenticate = proxy ?
        challenge.response.proxyAuthenticate :
        challenge.response.wwwAuthenticate;
    AuthParams &authorization = proxy ?
        nextRequest.request.proxyAuthorization :
        nextRequest.request.authorization;
    const AuthParams *auth;
    for (ChallengeList::const_iterator it = authenticate.begin();
        it != authenticate.end();
        ++it) {
        if (stricmp(it->scheme.c_str(), "Negotiate") == 0) {
            auth = &*it;
            break;
        }
        if (stricmp(it->scheme.c_str(), "NTLM") == 0) {
            auth = &*it;
            // Don't break; keep looking for Negotiate; it's preferred
        }
    }
    MORDOR_ASSERT(auth);
    return authorize(*auth, authorization, nextRequest.requestLine.uri);
}

}}
