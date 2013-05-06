// Downloaded from http://www.codeproject.com/Articles/13081/Controlling-Skype-with-C
// Altered by John MacCormick 2012, and re-released under existing CPOL 1.02 License
using System;
using System.Collections.Generic;
using System.Text;

namespace SkypeControl
{
    public class SkypeAttachEventArgs : EventArgs
    {
        public SkypeAttachStatus AttachStatus;

        public SkypeAttachEventArgs(SkypeAttachStatus theAttachStatus)
        {
            AttachStatus = theAttachStatus;
        }
    }

    public delegate void SkypeAttachHandler(object theSender, SkypeAttachEventArgs theEventArgs);

    public class SkypeResponseEventArgs : EventArgs
    {
        public string Response;

        public SkypeResponseEventArgs(string theResponse)
        {
            Response = theResponse;
        }
    }

    public delegate void SkypeResponseHandler(object theSender, SkypeResponseEventArgs theEventArgs);

    public class SkypeCommandEventArgs : EventArgs
    {
        public string Command;

        public SkypeCommandEventArgs(string theCommand)
        {
            Command = theCommand;
        }
    }

    public delegate void SkypeCommandHandler(object theSender, SkypeCommandEventArgs theEventArgs);

    public class SkypeProxyMessageEventArgs : EventArgs
    {
        public SkypeProxyMessageEventArgs(string s) { Message = s; }
        public string Message { get; private set; }
    }

    public delegate void SkypeProxyMessageHandler(object sender, SkypeProxyMessageEventArgs args);


    public class SkypeProxyNumCamerasEventArgs : EventArgs
    {
        public SkypeProxyNumCamerasEventArgs(int numCameras) { NumCameras = numCameras; }
        public int NumCameras { get; private set; }
    }

    public delegate void SkypeProxyNumCamerasHandler(object sender, SkypeProxyNumCamerasEventArgs args);


    public enum SkypeAttachStatus : uint
    {
        Success = 0,
        PendingAuthorization = 1,
        Refused = 2,
        NotAvailable = 3,
        Available = 0x8001
    }

    public class Constants
    {
        public const string SkypeControlAPIDiscover = "SkypeControlAPIDiscover";
        public const string SkypeControlAPIAttach = "SkypeControlAPIAttach";

        public const string PING_string = "PING";
        public const string PONG_string = "PONG";
        public const string CREATE_APPLICATION_string = "CREATE APPLICATION multicam";
        public const string CONNECT_APPLICATION_string = "ALTER APPLICATION multicam CONNECT ";
        public const string DISCONNECT_APPLICATION_string = "ALTER APPLICATION multicam DISCONNECT ";
        public const string REMOTE_USER_string = "dickinsoncomputerscience";
        public const string LOCAL_USER_string = "arawatabill";
        public const string GET_USER_request = "GET CURRENTUSERHANDLE";
        public const string GET_USER_response = "CURRENTUSERHANDLE ";
        public const string STREAM_NAME_string = "APPLICATION multicam STREAMS ";
        public const string WRITE_APPLICATION_string = "ALTER APPLICATION multicam WRITE ";
        public const string APPLICATION_RECEIVED_string = "APPLICATION multicam RECEIVED ";
        public const string READ_APPLICATION_string = "ALTER APPLICATION multicam READ ";
        public const string CALL_string = "CALL ";
        public const string STATUS_string = "STATUS ";
        public const string INPROGRESS_string = "INPROGRESS";
        public const string ROUTING_string = "ROUTING";
        public const string DURATION_string = " DURATION ";
        public const string PARTNER_string = "PARTNER_HANDLE";
        public const string FINISHED_string = "FINISHED";
        public const string GET_string = "GET ";
        public const string CHATMESSAGE_string_start = "MESSAGE ";
        public const string CHATMESSAGE_string_end = " STATUS RECEIVED";

        // MultiCam Ap2Ap protocol
        public const string AP2AP_VERSION_value = "1.1";
        public const string AP2AP_ADVANCE_CAMERA_request = "AP2AP_ADVANCE_CAMERA";
        public const string AP2AP_PING_request = "AP2AP_PING";
        public const string AP2AP_PONG_response = "AP2AP_PONG";
        public const string AP2AP_NUM_CAMERAS_request = "AP2AP_ASK_NUMCAMS";
        public const string AP2AP_NUM_CAMERAS_response = "AP2AP_REPLY_NUMCAMS ";
        public const string AP2AP_VERSION_request = "AP2AP_ASK_VERSION";
        public const string AP2AP_VERSION_response = "AP2AP_REPLY_VERSION ";

    }

}
