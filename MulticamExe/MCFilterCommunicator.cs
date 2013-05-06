// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Runtime.InteropServices;
using System.Diagnostics;
using System.Threading;

using SkypeControl;

namespace MultiCam
{
//#define HWND_BROADCAST  ((HWND)0xffff)
    public partial class MCFilterCommunicator : Form
    {
        [DllImport("user32.dll")]
        public static extern UInt32 RegisterWindowMessage(string lpString);

        [DllImport("user32.dll")]
        public static extern IntPtr SendNotifyMessage(IntPtr windowHandle,
            uint Msg,
            IntPtr wParam,
            IntPtr lParam);


        public struct Status
        {
            public bool isConnected;
            public int numCameras;

            public override string ToString()
            {
                string status = "";
                if (isConnected)
                {
                    status += ("The multicam filter is connected to " + numCameras + " local cameras\r\n");
                }
                else
                {
                    status += "The multicam filter is not connected\r\n";
                }
                return status;
            }
        
        }



        private const string MULTICAM_DISCOVER = "MulticamDiscover4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_ATTACH = "MulticamAttach4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_PRESSED = "MulticamAdvance4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_KICK = "MulticamKick4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_PING = "MulticamPing4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_PONG = "MulticamPong4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";
        private const string MULTICAM_RESET = "MulticamReset4AD2E57A-AF70-42AE-9A64-BC88F995B9C8";

        private UInt32 DiscoverMessageId = 0;
        private UInt32 AttachMessageId = 0;
        private UInt32 PressedMessageId = 0;
        private UInt32 KickMessageId = 0;
        private UInt32 PingMessageId = 0;
        private UInt32 PongMessageId = 0;
        private UInt32 ResetMessageId = 0;
        
        private IntPtr myMCFilterHandle = IntPtr.Zero;
        public bool IsConnected { get; private set; }
        private const int NUM_CAMERAS_INVALID = -1;
        private int numCameras = NUM_CAMERAS_INVALID;
        public int NumCameras {
            get { return numCameras; }
            private set
            {
                bool changed = (numCameras == value);
                numCameras = value;
                if (changed)
                {
                    RaiseNumCameras(numCameras);
                }
            }
        }

        // for debugging only
        IntPtr handle;

        //public event MulticamAttachHandler MulticamAttach;
        //public event SkypeResponseHandler SkypeResponse;
        //public event SkypeCommandHandler SkypeCommand;
        public event MessageEventHandler Message;
        public event EventHandler PongHandler;
        public event EventHandler ConnectHandler;
        public event EventHandler DisconnectHandler;
        public event MCFilterNumCamerasHandler NumCamerasHandler;

        private AutoResetEvent PongEvent = new AutoResetEvent(false);

        public MCFilterCommunicator()
        {
            InitializeComponent();
            SetDisconnected();
            DiscoverMessageId = RegisterWindowMessage(MULTICAM_DISCOVER);
            AttachMessageId = RegisterWindowMessage(MULTICAM_ATTACH);
            PressedMessageId = RegisterWindowMessage(MULTICAM_PRESSED);
            KickMessageId = RegisterWindowMessage(MULTICAM_KICK);
            PingMessageId = RegisterWindowMessage(MULTICAM_PING);
            PongMessageId = RegisterWindowMessage(MULTICAM_PONG);
            ResetMessageId = RegisterWindowMessage(MULTICAM_RESET);
            CreateHandle();
        }

        protected override void WndProc(ref Message m)
        {
            UInt32 aMsg = (UInt32)m.Msg;

            if (aMsg == DiscoverMessageId)
            {
                RaiseMessage(String.Format("MCFilterCommunicator windows procedure received discover message from camera DLL: {0}, {1}", 
                    m.WParam, m.LParam));
                //IntPtr ok = 
                //SendNotifyMessage(m.WParam, 
                    //(uint)AttachKeystrokeMessageId, this.Handle, new IntPtr(Multicam_keystroke_API.ATTACH_SUCCESS));
                //IntPtr aRetVal = SendNotifyMessage(m.WParam, (uint)AttachKeystrokeMessageId, this.Handle, IntPtr.Zero);
                lock (this)
                {
                    myMCFilterHandle = m.WParam;
                    NumCameras = (int) m.LParam;
                    IsConnected = true;
                    handle = this.Handle;
                }

                RaiseConnect();

                IntPtr aRetVal = SendNotifyMessage(m.WParam,
                    AttachMessageId, this.Handle, (IntPtr)Multicam_keystroke_API.ATTACH_SUCCESS);
                Debug.Assert(aRetVal != IntPtr.Zero);
                m.Result = new IntPtr(1);
                return;
            }
            else if (aMsg == PongMessageId)
            {
                RaiseMessage(String.Format("MCFilterCommunicator windows procedure received pong message from camera DLL: {0}", m.WParam));
                RaisePong();
                PongEvent.Set();
                m.Result = new IntPtr(1);
                return;
            }

            base.WndProc(ref m);
        }

        protected virtual void RaiseMessage(string message)
        {
            if (Message != null)
                Message(this, new MessageEventArgs(message));
        }

        protected virtual void RaisePong()
        {
            if (PongHandler != null)
                PongHandler(this, new EventArgs());
        }

        protected virtual void RaiseDisconnect()
        {
            if (DisconnectHandler != null)
                DisconnectHandler(this, new EventArgs());
        }

        protected virtual void RaiseConnect()
        {
            if (ConnectHandler != null)
                ConnectHandler(this, new EventArgs());
        }

        public void SendAdvanceCamera()
        {
            IntPtr handle;
            lock (this) { handle = myMCFilterHandle; }
            IntPtr aRetVal = SendNotifyMessage(handle,
                PressedMessageId, IntPtr.Zero, IntPtr.Zero);
            //Debug.Assert(aRetVal != IntPtr.Zero);
            if (aRetVal == IntPtr.Zero)
            {
                RaiseMessage(String.Format("An error occurred when notifying the MultiCam filter library" + 
                    " of the advance camera keystroke.\r\n" + 
                    "This probably means that Skype video is not currently running,\r\n" + 
                    "or the video started without MultiCam sensing it."));                 
            }
        }

        public void SendPing()
        {
            IntPtr handle;
            lock (this) { handle = myMCFilterHandle; }
            IntPtr aRetVal = SendNotifyMessage(handle,
                PingMessageId, IntPtr.Zero, IntPtr.Zero);
            //Debug.Assert(aRetVal != IntPtr.Zero);
            if (aRetVal == IntPtr.Zero)
            {
                RaiseMessage(String.Format("An error occurred when notifying the MultiCam filter library" +
                    " of a ping request.\r\n" +
                    "This probably means that Skype video is not currently running,\r\n" + 
                    "or the video started without MultiCam sensing it."));                 
            }
        }

        public void SendKick()
        {
            RaiseMessage("'Kick' message broadcast being sent...");
            Thread.Sleep(50);

            IntPtr result;
            IntPtr aRetVal = Platform.SendMessageTimeout(Platform.HWND_BROADCAST,
                KickMessageId, IntPtr.Zero, IntPtr.Zero,
                Platform.SendMessageTimeoutFlags.SMTO_ABORTIFHUNG,
                1000, out result);

            if (aRetVal == IntPtr.Zero)
            {
                RaiseMessage(String.Format("An error occurred when broadcasting the 'kick' message."));
            }
            else
            {
                RaiseMessage("'Kick' message broadcast successfully");
            }
        }

        public void SendReset()
        {
            IntPtr handle;
            lock (this) { handle = myMCFilterHandle; }
            IntPtr aRetVal = SendNotifyMessage(handle,
                ResetMessageId, IntPtr.Zero, IntPtr.Zero);
            //Debug.Assert(aRetVal != IntPtr.Zero);
            if (aRetVal == IntPtr.Zero)
            {
                RaiseMessage(String.Format("An error occurred when notifying the MultiCam filter library" +
                    " of a reset request.\r\n" +
                    "This probably means that Skype video is not currently running,\r\n" +
                    "or the video started without MultiCam sensing it."));
            }
        }

        public void CheckStatus()
        {
            bool gotPong = false;
            bool isConnected;
            lock (this) { isConnected = IsConnected; }
            if (isConnected)
            {
                PongEvent.Reset();
                SendPing();
                gotPong = PongEvent.WaitOne(1000);
            }
            if (!gotPong)
            {
                lock (this)
                {
                    SetDisconnected();
                }
                RaiseDisconnect();
                SendKick();
            }
        }

        private void SetDisconnected()
        {
            IsConnected = false;
            myMCFilterHandle = IntPtr.Zero;
            numCameras = NUM_CAMERAS_INVALID;
        }

        public Status GetStatus()
        {
            Status status = new Status();
            status.isConnected = IsConnected;
            status.numCameras = NumCameras;
            return status;
        }

        protected virtual void RaiseNumCameras(int numCameras)
        {
            if (NumCamerasHandler != null)
                NumCamerasHandler(this, new MCFilterNumCamerasEventArgs(numCameras));
        }


    }

    public enum Multicam_keystroke_API : uint
    {
        ATTACH_SUCCESS = 0,
        ATTACH_NOT_ATTEMPTED = 1
    }

    public class MulticamAttachEventArgs : EventArgs
    {
        public Multicam_keystroke_API AttachStatus;

        public MulticamAttachEventArgs(Multicam_keystroke_API theAttachStatus)
        {
            AttachStatus = theAttachStatus;
        }
    }

    public delegate void MulticamAttachHandler(object theSender, MulticamAttachEventArgs theEventArgs);

    public class MessageEventArgs : EventArgs
    {
        public MessageEventArgs(string s) { Message = s; }
        public string Message { get; private set; }
    }

    public delegate void MessageEventHandler(object sender, MessageEventArgs args);

    public class MCFilterNumCamerasEventArgs : EventArgs
    {
        public MCFilterNumCamerasEventArgs(int numCameras) { NumCameras = numCameras; }
        public int NumCameras { get; private set; }
    }

    public delegate void MCFilterNumCamerasHandler(object sender, MCFilterNumCamerasEventArgs args);

}
