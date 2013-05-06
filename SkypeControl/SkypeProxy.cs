// Downloaded from http://www.codeproject.com/Articles/13081/Controlling-Skype-with-C
// Altered by John MacCormick 2012, and re-released under existing CPOL 1.02 License
using System;
using System.Collections.Generic;
using System.Text;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

namespace SkypeControl
{
    public class SkypeProxy
    {
        public struct Status
        {
            public bool isSkypeRunning;
            public bool isSkypeConnected;
            public bool isSkypeOnACall;
            public bool isAp2ApCreated;
            public bool isAp2ApConnected;
            public string localUser;
            public string partner;
            public int numRemoteCameras;
            public string remoteProtocolVersion;
            public string remoteProgramVersion;

            public override string ToString()
            {
                string status = "";
                if (isSkypeRunning)
                {
                    status += "Skype is running on this computer\r\n";
                }
                else
                {
                    status += "Skype is not running on this computer\r\n";
                    return status;
                }


                if (isSkypeConnected)
                {
                    status += "MultiCam is connected to the Skype desktop protocol\r\n";
                }
                else
                {
                    status += "MultiCam is not connected to the Skype desktop protocol\r\n";
                    return status;
                }

                if (localUser != null && localUser != "")
                {
                    status += "The Skype user is logged in as " + localUser + "\r\n";
                }
                else
                {
                    status += "No Skype user is currently logged in\r\n";
                }

                if (isAp2ApCreated)
                {
                    status += "MultiCam has successfully created a Skype AP2AP session\r\n";
                }
                else
                {
                    status += "MultiCam has not successfully created a Skype AP2AP session\r\n";
                }

                if (isSkypeOnACall)
                {
                    status += "Skype is currently on a call ";
                }
                else
                {
                    status += "Skype is not currently on a call\r\n";
                    return status;
                }

                if (partner != null && partner != "")
                {
                    status += "with " + partner + "\r\n";
                }
                else
                {
                    status += "with an unknown Skype user\r\n";
                }

                if (isAp2ApConnected)
                {
                    status += "MultiCam has successfully connected to the remote Skype AP2AP session\r\n";
                    if (numRemoteCameras == NUM_CAMERAS_INVALID)
                    {
                        status += "The remote MultiCam filter is not connected\r\n";

                    }
                    else
                    {
                        status += "The remote MultiCam filter has " + numRemoteCameras + " cameras\r\n";
                    }
                }
                else
                {
                    status += "MultiCam has not successfully connected to the remote Skype AP2AP session\r\n";
                }

                if (remoteProtocolVersion != null && remoteProtocolVersion.Length > 0)
                {
                    status += "The remote version of the MultiCam protocol is " + remoteProtocolVersion + "\r\n";
                }

                if (remoteProgramVersion != null && remoteProgramVersion.Length > 0)
                {
                    status += "The remote version of the MultiCam program is " + remoteProgramVersion + "\r\n";
                }

                return status;
            }
        }

        public SkypeProxy()
        {
            mySkypeClient.SkypeAttach += new SkypeAttachHandler(mySkypeClient_OnSkypeAttach);
            mySkypeClient.SkypeResponse += new SkypeResponseHandler(mySkypeClient_OnSkypeResponse);
            mySkypeClient.SkypeCommand += new SkypeCommandHandler(mySkypeClient_OnSkypeCommand);

            skypeSuccess = new ManualResetEvent(false);
            ap2ApCreateEvent = new ManualResetEvent(false);
            ap2ApConnectEvent = new ManualResetEvent(false);
            ap2ApPongEvent = new ManualResetEvent(false);
            ap2ApNumCamerasEvent = new ManualResetEvent(false);
            ap2ApVersionEvent = new ManualResetEvent(false);
            getUserSuccess = new ManualResetEvent(false);
            pongSuccess = new ManualResetEvent(false);
            durationMessageEvent = new ManualResetEvent(false);
            partnerResponseEvent = new ManualResetEvent(false);

            WasSkypeRunning = false;
            WasSkypeConnected = false;
            WasSkypeOnACall = false;
            WasAp2ApCreated = false;
            WasAp2ApConnected = false;
        }

        public event SkypeAttachHandler SkypeAttach;
        public event SkypeResponseHandler SkypeResponse;
        public event SkypeCommandHandler SkypeCommand;
        public event SkypeProxyMessageHandler SkypeProxyMessage;
        public event SkypeProxyNumCamerasHandler NumCamerasHandler;
        public event EventHandler NumCamerasRequestHandler;
        public event EventHandler VersionRequestHandler;
        public event EventHandler AdvanceCameraHandler;
        public event EventHandler Ap2ApConnectHandler;

        protected ManualResetEvent skypeSuccess;
        protected ManualResetEvent ap2ApCreateEvent;
        protected ManualResetEvent ap2ApConnectEvent;
        protected ManualResetEvent ap2ApPongEvent;
        protected ManualResetEvent ap2ApNumCamerasEvent;
        protected ManualResetEvent ap2ApVersionEvent;
        protected ManualResetEvent getUserSuccess;
        protected ManualResetEvent pongSuccess;
        protected ManualResetEvent durationMessageEvent;
        protected ManualResetEvent partnerResponseEvent;

        public bool WasSkypeRunning {get; private set;}
        public bool WasSkypeConnected { get; private set; }
        public bool WasSkypeOnACall { get; private set; }
        public bool WasAp2ApCreated { get; private set; }
        public bool WasAp2ApConnected { get; private set; }

        private const int NUM_CAMERAS_INVALID = -1;

        protected int commandID = 0;
        protected string localUser = "";
        protected int callId = 0;
        protected bool initiatedCall = false;
        protected string partner = "";
        protected string streamName = "";
        protected int numRemoteCameras = NUM_CAMERAS_INVALID;
        protected int NumRemoteCameras
        {
            get { return numRemoteCameras; }
            private set
            {
                bool changed = (numRemoteCameras == value);
                numRemoteCameras = value;
                if (changed)
                {
                    RaiseNumCameras(numRemoteCameras);
                }
            }
        }
        protected string remoteProgramVersion = "";
        protected string remoteProtocolVersion = "";
        
        protected bool chatMsgAdvancesCamera = true;
        public bool ChatMsgAdvancesCamera
        {
            get { return chatMsgAdvancesCamera; }
            set { chatMsgAdvancesCamera = value; }
        }
        

        protected string getNewCommandID()
        {
            commandID++;
            return "#" + commandID + " ";
        }

        protected virtual void RaiseSkypeProxyMessage(string message)
        {
            if (SkypeProxyMessage != null)
                SkypeProxyMessage(this, new SkypeProxyMessageEventArgs(message));
        }

        protected virtual void RaiseNumCameras(int numCameras)
        {
            if (NumCamerasHandler != null)
                NumCamerasHandler(this, new SkypeProxyNumCamerasEventArgs(numCameras));
        }

        protected virtual void RaiseNumCamerasRequest()
        {
            if (NumCamerasRequestHandler != null)
                NumCamerasRequestHandler(this, new EventArgs());
        }

        protected virtual void RaiseVersionRequest()
        {
            if (VersionRequestHandler != null)
                VersionRequestHandler(this, new EventArgs());
        }

        protected virtual void RaiseAdvanceCamera()
        {
            if (AdvanceCameraHandler != null)
                AdvanceCameraHandler(this, new EventArgs());
        }

        protected virtual void RaiseAp2ApConnect()
        {
            if (Ap2ApConnectHandler != null)
                Ap2ApConnectHandler(this, new EventArgs());
        }

        private void mySkypeClient_OnSkypeAttach(object theSender, SkypeAttachEventArgs theEventArgs)
        {
            if (SkypeAttach != null)
                SkypeAttach(this, theEventArgs);

            if (theEventArgs.AttachStatus == SkypeAttachStatus.Success)
            {
                skypeSuccess.Set();
            //    Thread thread = new Thread(new ThreadStart(ConnectAp2Ap));
            //    thread.Name = "ConnectAp2Ap";
            //    thread.Start();
            }
        }

        public static bool IsDurationMessage(string message)
        {
            return (message.StartsWith(Constants.CALL_string)
                && message.Contains(Constants.DURATION_string));
        }

        void DisconnectStream()
        {
            if (streamName != "")
            {
                Command(Constants.DISCONNECT_APPLICATION_string + streamName);
                streamName = "";
            }
        }

        void HandleCallMessage(string message)
        {


            string remainder;
            int callId = ExtractCallId(message, out remainder);

            bool isNewCall = false;
            if (callId != this.callId)
            {
                // new call
                isNewCall = true;
                DoCallStarted(callId);
                initiatedCall = false;
                RaiseSkypeProxyMessage(String.Format("    HandleCallMessage: new call {0}", callId));
            }

            if (IsDurationMessage(message))
            {
                if (isNewCall)
                {
                    SendPartnerRequest();
                }
                durationMessageEvent.Set();
                return;
            }

            RaiseSkypeProxyMessage(String.Format("    HandleCallMessage: remainder is '{0}'", remainder));

            if (remainder.StartsWith(Constants.STATUS_string))
            {
                string status = remainder.Substring(Constants.STATUS_string.Length);
                if (status == Constants.ROUTING_string)
                {
                    initiatedCall = true;
                    RaiseSkypeProxyMessage("    HandleCallMessage: Recorded call as initiated");
                }
                else if (status == Constants.INPROGRESS_string)
                {
                    if (initiatedCall)
                    {
                        SendPartnerRequest();
                    }
                }
                else if (status == Constants.FINISHED_string)
                {
                    DisconnectStream();
                    DoCallEnded();
                }
            }
            else if (remainder.StartsWith(Constants.PARTNER_string))
            {
                ProcessPartnerResponse(remainder);
            }
        }

        private int ExtractCallId(string message, out string remainder)
        {
            int id_start_loc = Constants.CALL_string.Length;
            int id_end_loc = message.IndexOf(' ', id_start_loc);
            remainder = message.Substring(id_end_loc + 1);
            int id_len = id_end_loc - id_start_loc;
            string callIdStr = message.Substring(id_start_loc, id_len);
            int callId = int.Parse(callIdStr);
            return callId;
        }

        private void DoCallStarted(int callId)
        {
            this.partner = "";
            this.callId = callId;
            this.WasSkypeOnACall = true;
        }

        protected void ProcessPartnerResponse(string remainder)
        {
            int partner_start_loc = Constants.PARTNER_string.Length + 1;
            string partner = remainder.Substring(partner_start_loc);
            this.partner = partner;
            partnerResponseEvent.Set();
        }

        private void ConnectAp2Ap()
        {
            // disconnect any existing stream (There shouldn't be one, really,
            // unless we failed to detect the end of a previous call.)
            DisconnectStream();
            // connect new stream
            if (partner != "")
            {
                Command(Constants.CONNECT_APPLICATION_string + partner);
            }
            else
            {
                RaiseSkypeProxyMessage("ConnectAp2Ap: partner is empty,  so can't connect Ap2Ap");
            }
        }

        protected void SendPartnerRequest()
        {
            partnerResponseEvent.Reset();
            string request = Constants.GET_string + Constants.CALL_string + callId + " " + Constants.PARTNER_string;
            RaiseSkypeProxyMessage(String.Format("    SendPartnerRequest: sending Skype request '{0}'", request));
            Command(request);
        }

        void mySkypeClient_OnSkypeResponse(object theSender, SkypeResponseEventArgs theEventArgs)
        {
            if (SkypeResponse != null)
                SkypeResponse(this, theEventArgs);

            String response = theEventArgs.Response;
            // strip off any command ID
            if (response[0] == '#')
            {
                response = response.Substring(response.IndexOf(' ') + 1);
            }

            if (response.StartsWith(Constants.CALL_string))
            {
                HandleCallMessage(response);
            }
            else if (response == Constants.PONG_string)
            {
                pongSuccess.Set();
            }
            else if (response == Constants.CREATE_APPLICATION_string)
            {
                ap2ApCreateEvent.Set();
            }
            else if (response.StartsWith(Constants.GET_USER_response))
            {
                localUser = response.Substring(Constants.GET_USER_response.Length);
                getUserSuccess.Set();
            }
            else if (response.StartsWith(Constants.STREAM_NAME_string))
            {
                if (response.Length > Constants.STREAM_NAME_string.Length)
                {
                    streamName = response.Substring(Constants.STREAM_NAME_string.Length);
                    RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: stream name is '{0}'", streamName));
                    RaiseAp2ApConnect();
                    ap2ApConnectEvent.Set();
                }
                else
                {
                    RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: received empty stream name, so do nothing"));
                }
            }
            else if (response.StartsWith(Constants.APPLICATION_RECEIVED_string))
            {
                HandleApplicationReceived(response);
            }
            else if (response.StartsWith(Constants.READ_APPLICATION_string))
            {
                HandleReadApplication(response);
            }
            else if (response.StartsWith(Constants.CHATMESSAGE_string_start)
                && response.EndsWith(Constants.CHATMESSAGE_string_end))
            {
                if (ChatMsgAdvancesCamera)
                {
                    RaiseSkypeProxyMessage("CHATMESSAGE received -- will switch cameras");
                    RaiseAdvanceCamera();
                }
                else
                {
                    RaiseSkypeProxyMessage("CHATMESSAGE received, but camera-switching disabled for chat messages");
                }
            }

        }

        private void HandleReadApplication(String response)
        {
            int startPos = Constants.READ_APPLICATION_string.Length;
            int spacePos = response.IndexOf(' ', startPos);
            Debug.Assert(spacePos != -1);
            int length = spacePos - startPos;
            string streamName = response.Substring(startPos, length);
            Debug.Assert(streamName == this.streamName);
            startPos += (length + 1);
            string dataStr = response.Substring(startPos);
            RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: Got READ message; data is '{0}'", dataStr));
            if (dataStr == Constants.AP2AP_ADVANCE_CAMERA_request)
            {
                RaiseSkypeProxyMessage("ADVANCE_CAMERA received -- need to switch cameras");
                RaiseAdvanceCamera();
            }
            else if (dataStr == Constants.AP2AP_PING_request)
            {
                RaiseSkypeProxyMessage("AP2AP_PING received -- will send pong");
                SendAp2ApMessage(Constants.AP2AP_PONG_response);
            }
            else if (dataStr == Constants.AP2AP_PONG_response)
            {
                RaiseSkypeProxyMessage("AP2AP_PONG received");
                ap2ApPongEvent.Set();
            }
            else if (dataStr.StartsWith(Constants.AP2AP_NUM_CAMERAS_response))
            {
                RaiseSkypeProxyMessage("AP2AP_NUM_CAMERAS received");
                string numCameras_string = dataStr.Substring(Constants.AP2AP_NUM_CAMERAS_response.Length);
                NumRemoteCameras = Int32.Parse(numCameras_string);
                RaiseSkypeProxyMessage(
                    String.Format("    ... number of remote cameras was {0}", NumRemoteCameras));
                ap2ApNumCamerasEvent.Set();
            }
            else if (dataStr == Constants.AP2AP_NUM_CAMERAS_request)
            {
                RaiseSkypeProxyMessage("AP2AP_NUM_CAMERAS request received");
                RaiseNumCamerasRequest();
            }
            else if (dataStr == Constants.AP2AP_VERSION_request)
            {
                RaiseSkypeProxyMessage("AP2AP_VERSION request received");
                RaiseVersionRequest();
            }
            else if (dataStr.StartsWith(Constants.AP2AP_VERSION_response))
            {
                RaiseSkypeProxyMessage("AP2AP_VERSION response received");
                string version_string = dataStr.Substring(Constants.AP2AP_VERSION_response.Length);
                string[] tokens = version_string.Split(' ');
                if (tokens.Length != 2)
                {
                    RaiseSkypeProxyMessage("Error: Malformed AP2AP_VERSION response");
                }
                else
                {
                    remoteProtocolVersion = tokens[0];
                    remoteProgramVersion = tokens[1];
                    RaiseSkypeProxyMessage(
                    String.Format("    ... Remote protocol version is {0}; Remote program version is {1}",
                        remoteProtocolVersion, remoteProgramVersion));
                }
                ap2ApVersionEvent.Set();
            }
            //if (m_hKeystrokeMessageWindow != NULL)
            //{
            //    RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: sending this data value to window 0x%x", m_hKeystrokeMessageWindow);
            //    ok = SendNotifyMessage(m_hKeystrokeMessageWindow, m_uiPressedKeystrokeMessageId,
            //        (WPARAM) dataVal, 0);
            //    Debug.Assert(ok);
            //}
        }

        private void HandleApplicationReceived(String response)
        {
            int startPos = Constants.APPLICATION_RECEIVED_string.Length;
            int equalPos = response.IndexOf('=');
            if (equalPos == -1)
            {
                RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: Got RECEIVED notification, but no stream name, so do nothing"));
            }
            else
            {
                int length = equalPos - startPos;
                string streamName = response.Substring(startPos, length);
                if (this.streamName == streamName)
                {
                    RaiseSkypeProxyMessage(String.Format("SkypeApi::HandleSkypeMessage: Got RECEIVED notification with matching stream name, will now attempt to read"));
                    Command(Constants.READ_APPLICATION_string + streamName);
                }
                else
                {
                    RaiseSkypeProxyMessage(String.Format(
                        "SkypeApi::HandleSkypeMessage: Got RECEIVED notification with incompatible stream name,"
                        + " so do nothing\r\n" +
                        "(this.streamName = {0}; streamName = {1})",
                        this.streamName, streamName));
                }
            }
        }

        public void TellNumCameras(int numCameras)
        {
            SendAp2ApMessage(Constants.AP2AP_NUM_CAMERAS_response + numCameras.ToString());
        }

        void mySkypeClient_OnSkypeCommand(object theSender, SkypeCommandEventArgs theEventArgs)
        {
            if (SkypeCommand != null)
                SkypeCommand(this, theEventArgs);
        }

        private SkypeClient mySkypeClient = new SkypeClient();

        public bool Connect()
        {
            return mySkypeClient.Connect();
        }

        public void Disconnect()
        {
            mySkypeClient.Disconnect();
            skypeSuccess.Reset();
            WasSkypeConnected = false;
        }

        public void Command(string theCommand)
        {
            mySkypeClient.Command(getNewCommandID() + theCommand);
        }

        public void Ping()
        {
            Command(Constants.PING_string);
        }

        //public void CreateAp2Ap()
        //{
        //    Thread thread = new Thread(new ThreadStart(DoCreateAp2Ap));
        //    thread.Name = "CreateAp2Ap";
        //    thread.Start();
        //}

        protected bool DoCreateAp2Ap()
        {
            bool ok = false;
            //while (!ok)
            //{
            //    ok = skypeSuccess.WaitOne(2000);
            //    if (!ok)
            //    {
            //        RaiseSkypeProxyMessage("Ap2Ap creation is waiting for connection to Skype; retrying that connection now");
            //        Connect();
            //    }
            //}
            ap2ApCreateEvent.Reset();
            Command(Constants.CREATE_APPLICATION_string);
            ok = ap2ApCreateEvent.WaitOne(3000);
            if (!ok)
            {
                RaiseSkypeProxyMessage("Timed out waiting for Ap2Ap creation");
            }
            else
            {
                RaiseSkypeProxyMessage("Ap2Ap creation succeeded");
                WasAp2ApCreated = true;
            }
            return ok;
        }

        private void DetermineLocalUser()
        {
            getUserSuccess.Reset();
            Command(Constants.GET_USER_request);
            bool ok = getUserSuccess.WaitOne(3000);
            if (!ok)
            {
                RaiseSkypeProxyMessage("Timed out waiting for user information");
                return;
            }
            RaiseSkypeProxyMessage(String.Format("Got user '{0}'", localUser));
        }

        public void SendAp2ApMessage(string str)
        {

            if (streamName.Length > 0)
            {
                string full_message = Constants.WRITE_APPLICATION_string + streamName + " " + str;
                Command(full_message);
            }
            else
            {
                RaiseSkypeProxyMessage(String.Format("SkypeApi::SendAp2ApMessage: Not currently connected, so not sending message '{0}'", str));
            }
        }

        private bool IsSkypeRunning()
        {
            Process[] processes = Process.GetProcessesByName("Skype");
            return (processes.Length > 0);
        }



        public void CheckStatus()
        {
            bool succeeded = false;
            if (!IsSkypeRunning())
            {
                RaiseSkypeProxyMessage(String.Format("SkypeApi::CheckStatus: Skype isn't running on this computer."));
                SetSkypeNotRunning();
                return;
            }

            WasSkypeRunning = true;

            if (!WasSkypeConnected)
            {
                skypeSuccess.Reset();
                this.Connect();
                succeeded = skypeSuccess.WaitOne(2000);
                if (!succeeded)
                {
                    RaiseSkypeProxyMessage(String.Format("SkypeApi::CheckStatus: Couldn't connect to Skype desktop protocol"));
                    SetSkypeDisconnected();
                    return;
                }
            }

            pongSuccess.Reset();
            this.Ping();
            succeeded = pongSuccess.WaitOne(2000);
            if (succeeded)
            {
                WasSkypeConnected = true;
            }
            else
            {
                RaiseSkypeProxyMessage(String.Format("SkypeApi::CheckStatus: Didn't receive ping response via Skype desktop protocol"));
                SetSkypeDisconnected();
                return;
            }

            DetermineLocalUser();

            if (!WasAp2ApCreated)
            {
                succeeded = DoCreateAp2Ap();
                if (!succeeded)
                {
                    SetAp2ApNotCreated();
                    return;
                }
            }

            durationMessageEvent.Reset();
            succeeded = durationMessageEvent.WaitOne(2000);
            if (!succeeded)
            {
                DoCallEnded();
                return;
            }

            // We are on a call...
            partnerResponseEvent.Reset();
            SendPartnerRequest();
            succeeded = partnerResponseEvent.WaitOne(2000);
            if (!succeeded)
            {
                DoCallEnded();
                return;
            }

            if (!WasAp2ApConnected)
            {
                ap2ApConnectEvent.Reset();
                ConnectAp2Ap();
                succeeded = ap2ApConnectEvent.WaitOne(2000);
                if (!succeeded)
                {
                    SetAp2ApDisconnected();
                    return;
                }
                else
                {
                    WasAp2ApConnected = true;
                }
            }

            // We are probably on a call and we know the partner ID.
            // Ap2Ap is probably connected. So ping Ap2Ap.
            ap2ApPongEvent.Reset();
            this.SendAp2ApMessage(Constants.AP2AP_PING_request);
            succeeded = ap2ApConnectEvent.WaitOne(3000);
            if (!succeeded)
            {
                SetAp2ApDisconnected();
                return;
            }
            else
            {
                WasAp2ApConnected = true;
            }


            ap2ApNumCamerasEvent.Reset();
            this.SendAp2ApMessage(Constants.AP2AP_NUM_CAMERAS_request);
            succeeded = ap2ApNumCamerasEvent.WaitOne(2000);
            if (!succeeded)
            {
                SetNumCamerasInvalid();
                return;
            }

            ap2ApVersionEvent.Reset();
            this.SendAp2ApMessage(Constants.AP2AP_VERSION_request);
            succeeded = ap2ApVersionEvent.WaitOne(2000);
            if (!succeeded)
            {
                RaiseSkypeProxyMessage("CheckStatus: Error: version request message received no response.");
                return;
            }

        }

        private void SetSkypeNotRunning()
        {
            WasSkypeRunning = false;
            SetSkypeDisconnected();
        }

        private void SetSkypeDisconnected()
        {
            WasSkypeConnected = false;
            SetAp2ApNotCreated();
        }
        
        private void SetAp2ApNotCreated()
        {
            WasAp2ApCreated = false;
            DoCallEnded();
        }

        private void DoCallEnded()
        {
            this.partner = "";
            this.callId = 0;
            WasSkypeOnACall = false;
            SetAp2ApDisconnected();
        }

        private void SetAp2ApDisconnected()
        {
            WasAp2ApConnected = false;
            SetNumCamerasInvalid();
        }

        private void SetNumCamerasInvalid()
        {
            NumRemoteCameras = NUM_CAMERAS_INVALID;
        }




        public Status GetStatus()
        {
            Status status = new Status();
            status.isSkypeRunning = WasSkypeRunning;
            status.isSkypeConnected = WasSkypeConnected;
            status.isSkypeOnACall = WasSkypeOnACall;
            status.isAp2ApCreated = WasAp2ApCreated;
            status.isAp2ApConnected = WasAp2ApConnected;
            status.localUser = localUser;
            status.partner = partner;
            status.numRemoteCameras = NumRemoteCameras;
            status.remoteProgramVersion = remoteProgramVersion;
            status.remoteProtocolVersion = remoteProtocolVersion;
            return status;
        }

        public void TellVersion(string protocolVersion, string programVersion)
        {
            SendAp2ApMessage(Constants.AP2AP_VERSION_response + protocolVersion + " " + programVersion);
        }
    }
}
