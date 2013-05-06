// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Diagnostics;
using System.Threading;

using SkypeControl;
using Gma.UserActivityMonitor;

using MultiCamHelpers;


namespace MultiCam
{
    public partial class Form1 : Form
    {
        public struct Status
        {
            public SkypeProxy.Status skypeProxyStatus;
            public MCFilterCommunicator.Status mcFilterStatus;

            public override string ToString()
            {
                return skypeProxyStatus.ToString() + mcFilterStatus.ToString();
            }
        }

        SkypeProxy skypeProxy = new SkypeProxy();
        delegate void AlterTextCallback(string text);
        delegate void EnableButtonCallback(bool shouldEnable);
        MCFilterCommunicator filterCommunicator;
        AutoResetEvent FormClosingNow = new AutoResetEvent(false);
        public bool ForceClosed {get; set;}

        Status status;
        private bool expandedWindow = false;

        public Form1()
        {
            MulticamLog.Log(0, String.Format("MultiCam application version {0}", this.ProductVersion));

            ForceClosed = false;
            InitializeComponent();

            CheckAdminPrivileges();
            //CheckSkypeRunning();

            filterCommunicator = new MCFilterCommunicator();

            filterCommunicator.Message += new MessageEventHandler(filterCommunicator_Message);
            filterCommunicator.ConnectHandler += new EventHandler(filterCommunicator_Connect);
            filterCommunicator.DisconnectHandler += new EventHandler(filterCommunicator_Disconnect);
            filterCommunicator.NumCamerasHandler += new MCFilterNumCamerasHandler(filterCommunicator_NumCameras);

            skypeProxy.SkypeAttach += new SkypeAttachHandler(skypeProxy_SkypeAttach);
            skypeProxy.SkypeResponse += new SkypeResponseHandler(skypeProxy_SkypeResponse);
            skypeProxy.SkypeCommand += new SkypeCommandHandler(skypeProxy_SkypeCommand);
            skypeProxy.SkypeProxyMessage += new SkypeProxyMessageHandler(skypeProxy_SkypeProxyMessage);
            skypeProxy.AdvanceCameraHandler += new EventHandler(skypeProxy_AdvanceCamera);
            skypeProxy.Ap2ApConnectHandler += new EventHandler(skypeProxy_Ap2ApConnect);
            skypeProxy.NumCamerasHandler += new SkypeProxyNumCamerasHandler(skypeProxy_NumCameras);
            skypeProxy.NumCamerasRequestHandler += new EventHandler(skypeProxy_NumCamerasRequest);
            skypeProxy.VersionRequestHandler += new EventHandler(skypeProxy_VersionRequest);
            //skypeProxy.Connect();
            //skypeProxy.ConnectAp2Ap();

            HookManager.KeyDown += HookManager_KeyDown;

            GetStatus();
            UpdateStatus(status.ToString());
            CheckStatusInNewThread();
        }

        protected override void OnClosing(CancelEventArgs e)
        {
            base.OnClosing(e);
            FormClosingNow.Set();
        }

        private static void CheckAdminPrivileges()
        {
            bool elevated = false;

            // UacHelper.IsProcessElevated throws an exception on Windows XP,
            // which we don't care too much about, so log it and continue
            try
            {
                elevated = UacHelper.IsProcessElevated;
            }
            catch (Exception e)
            {
                MulticamLog.Log(e);
            }

            if (elevated)
            {
                MessageBox.Show("This program is running with administrator privileges,\r\n" +
                    "and Skype will therefore refuse to connect to it.\r\n" +
                    "Please run the program again, without administrator privileges.",
                    "MultiCam");
                Environment.Exit(0);
            }
        }

        //private static void CheckSkypeRunning()
        //{
        //    bool done = false;
        //    while (!done)
        //    {
        //        Process[] processes = Process.GetProcessesByName("Skype");
        //        if (processes.Length == 0)
        //        {
        //            var result = MessageBox.Show("Skype does not appear to be running on this computer.\r\n" +
        //                "Please start Skype before continuing.",
        //                "MultiCam",
        //                MessageBoxButtons.OKCancel);
        //            if (result == DialogResult.Cancel)
        //            {
        //                Environment.Exit(0);    
        //            }
        //        }
        //        else
        //        {
        //            done = true;
        //        }
        //    }
        //}

        private void AppendText(string text)
        {


            //if (this.textBox1.InvokeRequired)
            //{
            //    AlterTextCallback d = new AlterTextCallback(AppendText);
            //    this.Invoke(d, new object[] { text });
            //}
            //else
            //{
                MulticamLog.Log(20, text);
            //    try
            //    {
            //        textBox1.AppendText(text);
            //    }
            //    catch (ObjectDisposedException)
            //    {
            //        // Deliberately do nothing -- the UI is gone anyway
            //    }                    
            //}
        }

        private void UpdateStatus(string text)
        {


            if (this.textBox1.InvokeRequired)
            {
                AlterTextCallback d = new AlterTextCallback(UpdateStatus);
                this.Invoke(d, new object[] { text });
            }
            else
            {
                try
                {
                    textBox1.Text = text;
                }
                catch (ObjectDisposedException)
                {
                    // Deliberately do nothing -- the UI is gone anyway
                }
            }
        }

        private void EnableRemoteSwitch(bool shouldEnable)
        {
            if (this.button4.InvokeRequired)
            {
                EnableButtonCallback d = new EnableButtonCallback(EnableRemoteSwitch);
                this.Invoke(d, new object[] { shouldEnable });
            }
            else
            {
                try
                {
                    button4.Enabled = shouldEnable;
                }
                catch (ObjectDisposedException)
                {
                    // Deliberately do nothing -- the UI is gone anyway
                }
            }
        }


        private void EnableLocalSwitch(bool shouldEnable)
        {


            if (this.button6.InvokeRequired)
            {
                EnableButtonCallback d = new EnableButtonCallback(EnableLocalSwitch);
                this.Invoke(d, new object[] { shouldEnable });
            }
            else
            {
                try
                {
                    button6.Enabled = shouldEnable;
                }
                catch (ObjectDisposedException)
                {
                    // Deliberately do nothing -- the UI is gone anyway
                }
            }
        }

        void skypeProxy_SkypeAttach(object theSender, SkypeAttachEventArgs theEventArgs)
        {
            this.AppendText(string.Format("Attach: {0}\r\n", theEventArgs.AttachStatus));
        }

        void skypeProxy_SkypeResponse(object theSender, SkypeResponseEventArgs theEventArgs)
        {
            string response = theEventArgs.Response;
            if (!(SkypeProxy.IsDurationMessage(response)))
                this.AppendText(string.Format("Response: {0}\r\n", response));
        }

        void skypeProxy_SkypeCommand(object theSender, SkypeCommandEventArgs theEventArgs)
        {
            this.AppendText(string.Format("Command: {0}\r\n", theEventArgs.Command));
        }

        void skypeProxy_SkypeProxyMessage(object theSender, SkypeProxyMessageEventArgs theEventArgs)
        {
            this.AppendText(string.Format("ProxyMessage: {0}\r\n", theEventArgs.Message));
        }

        void skypeProxy_NumCameras(object theSender, SkypeProxyNumCamerasEventArgs theEventArgs)
        {
            if (theEventArgs.NumCameras > 1)
            {
                EnableRemoteSwitch(true);
            }
            else
            {
                EnableRemoteSwitch(false);
            }
        }

        void skypeProxy_NumCamerasRequest(object theSender, EventArgs theEventArgs)
        {
            skypeProxy.TellNumCameras(filterCommunicator.NumCameras);
        }

        void skypeProxy_VersionRequest(object theSender, EventArgs theEventArgs)
        {
            skypeProxy.TellVersion(Constants.AP2AP_VERSION_value, this.ProductVersion);
        }

        void skypeProxy_AdvanceCamera(object theSender, EventArgs e)
        {
            AdvanceLocalCamera();
        }

        void skypeProxy_Ap2ApConnect(object theSender, EventArgs e)
        {
            skypeProxy.TellNumCameras(filterCommunicator.NumCameras);
        }

        void filterCommunicator_Message(object theSender, MessageEventArgs e)
        {
            this.AppendText(string.Format("filterCommunicator: {0}\r\n", e.Message));
        }

        void filterCommunicator_Connect(object theSender, EventArgs e)
        {
            if (filterCommunicator.NumCameras > 1)
            {
                EnableLocalSwitch(true);
            }
            else
            {
                EnableLocalSwitch(false);
            }

        }

        void filterCommunicator_Disconnect(object theSender, EventArgs e)
        {
            EnableLocalSwitch(false);
        }

        void filterCommunicator_NumCameras(object theSender, MCFilterNumCamerasEventArgs e)
        {
            if (e.NumCameras > 1)
            {
                EnableLocalSwitch(true);
            }
            else
            {
                EnableLocalSwitch(false);
            }
        }

        private void button3_Click(object sender, EventArgs e)
        {
            //try
            //{
                //int y = 0;
                //int x = 1 / y;
                //throw new System.IO.FileNotFoundException("adsf");
                throw new Exception("asdf");
                //skypeProxy.CreateAp2Ap();
            //}
            //catch (Exception)
            //{
            //    MessageBox.Show("alksdjf");
            //    throw;
            //}
        }



        private void HookManager_KeyDown(object sender, KeyEventArgs e)
        {
            AppendText(string.Format("KeyDown - {0}\n", e.KeyCode));
            Keys key = e.KeyCode;
            if (key == Keys.Space)
            {
                AdvanceRemoteCamera();
            }
            else if (key == Keys.Enter)
            {
                
                AdvanceLocalCamera();
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            HookManager.KeyDown -= HookManager_KeyDown;
        }

        private void checkBox1_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox1.Checked)
            {
                HookManager.KeyDown += HookManager_KeyDown;  
            } 
            else
            {
                HookManager.KeyDown -= HookManager_KeyDown;
            }
        }

        private void button6_Click(object sender, EventArgs e)
        {
            AdvanceLocalCamera();
        }

        private void AdvanceLocalCamera()
        {
            this.AppendText("Advancing local camera\r\n");
            filterCommunicator.SendAdvanceCamera();              
        }

        private void AdvanceRemoteCamera()
        {
            this.AppendText("Advancing remote camera\r\n");
            skypeProxy.SendAp2ApMessage(Constants.AP2AP_ADVANCE_CAMERA_request);
        }

        private void ResetMultiCamFilter()
        {
            this.AppendText("Resetting MultiCam filter\r\n");
            filterCommunicator.SendReset();
        }

        private void button4_Click(object sender, EventArgs e)
        {
            AdvanceRemoteCamera();
        }

        private void CheckStatusInNewThread()
        {
            Thread thread = new Thread(new ThreadStart(CheckStatus));
            thread.Name = "CheckStatus";
            thread.Start();
        }

        private void CheckStatus()
        {
            bool done = false;
            while (!done)
            {
                try
                {
                    if (ForceClosed) break;
                    filterCommunicator.CheckStatus();
                    if (ForceClosed) break;
                    skypeProxy.CheckStatus();

                    Status prevStatus = status;
                    GetStatus();
                    if (!status.Equals(prevStatus))
                    {
                        UpdateStatus(status.ToString());
                    }

                    if (ForceClosed) break;
                    done = FormClosingNow.WaitOne(5000);
                    if (ForceClosed) break;
                }
                catch (Exception e)
                {
                    if (!ForceClosed)
                    {
                        MulticamLog.Log(e);
                        //MessageBox.Show(
                        //"MultiCam encountered an unexpected problem " +
                        //"while checking its connections,\r\n" +
                        //"but this is probably a temporary problem, so MultiCam will continue as normal.");
                    }
                }

            }
        }

        public Status GetStatus()
        {
            status.skypeProxyStatus = this.skypeProxy.GetStatus();
            status.mcFilterStatus = this.filterCommunicator.GetStatus();
            return status;
        }

        private void checkBox2_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox2.Checked)
            {
                label1.Visible = true;
                textBox1.Visible = true;
                if (!expandedWindow)
                {
                    this.Width = textBox1.Width + 45;
                    this.Height += textBox1.Height + 40;
                    expandedWindow = true;
                }
            }
            else
            {
                label1.Visible = false;
                textBox1.Visible = false;
            }
        }

        private void button1_Click(object sender, EventArgs e)
        {
            ResetMultiCamFilter();
        }

        private void exitToolStripMenuItem_Click(object sender, EventArgs e)
        {
            this.Close();
        }

        private void aboutToolStripMenuItem_Click(object sender, EventArgs e)
        {
            MessageBox.Show("MultiCam version " + this.ProductVersion);
        }

        private void onlineHelpToolStripMenuItem_Click(object sender, EventArgs e)
        {
            System.Diagnostics.Process.Start("http://users.dickinson.edu/~jmac/multicam/");
        }

        private void checkBox3_CheckedChanged(object sender, EventArgs e)
        {
            if (checkBox3.Checked)
            {
                skypeProxy.ChatMsgAdvancesCamera = true;
            }
            else
            {
                skypeProxy.ChatMsgAdvancesCamera = false;
            }
        }


    }
}
