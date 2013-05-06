// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Windows.Forms;
using MultiCamHelpers;

namespace MultiCam
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main()
        {
            Form1 form1 = null;
            MulticamLog.Open();
            Application.SetUnhandledExceptionMode(UnhandledExceptionMode.ThrowException);
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);
            bool done = false;
            while (!done)
            {
                try
                {
                    form1 = new Form1();
                    Application.Run(form1);
                    done = true;
                }
                catch (Exception e)
                {
                    MulticamLog.Log(e);
                    MessageBox.Show("MultiCam encountered an unexpected problem, " +
                        "and will now close.\r\n" +
                        "Please accept our apologies!");
                    // Application.Restart() caused problems, so gave up on it.
                    form1.Close();
                    form1.ForceClosed = true;
                    Application.Exit();
                    MulticamLog.Log(0, "*********** MultiCam restarting ****************");
                }
            }
            MulticamLog.Close();
        }
    }
}
