// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Windows.Forms;

namespace MultiCamHelpers
{
    public static class MulticamLog
    {
        private const string logFileDir = @"C:\temp";
        private const string logFileNameCompt = @"multicam-log.txt";
        private static string logFileName = null;
        private const int verbosityLevel = 0;

        private static StreamWriter logFile = null;
        private static bool openedOK = false;

        public static void Open()
        {
            try
            {
                logFileName = Path.Combine(logFileDir, logFileNameCompt);

                if (!Directory.Exists(logFileDir))
                {
                    Directory.CreateDirectory(logFileDir);
                }

                FileInfo logFileInfo = new FileInfo(logFileName);

                if (File.Exists(logFileName) && logFileInfo.Length > 1024 * 1024)
                {
                    File.Delete(logFileName);
                }

                logFile = new StreamWriter(logFileName, true);
                logFile.AutoFlush = true;

                openedOK = true;

                Log(0, "************ MultiCam Log opened ***************");

            }
            catch (Exception e)
            {
                 MessageBox.Show("MultiCam encountered an unexpected problem " +
                        "while opening the log,\r\n" +
                        "so logging has been disabled.\r\n" +
                        "Details: " + e.Message);
                 openedOK = false;
            }
        }

        public static void Close()
        {
            Log(0, "~~~~~~~~~~~~~ MultiCam Log closed ~~~~~~~~~~~~~~");
            if (logFile != null)
            {
                openedOK = false;
                logFile.Close();
                logFile = null;
            }
        }

        public static void Log(int verbosity, string message)
        {
            if (openedOK && verbosity <= verbosityLevel)
            {
                DateTime now = DateTime.Now;

                Write(now + "+" + now.Millisecond + "ms: " + message.TrimEnd() + "\n");
            }
        }

        public static void Log(Exception exception)
        {
            if (openedOK)
            {
                Write(String.Format(
                    "EXCEPTION in {0} ({1}):\n{2}\n", exception.Source, 
                    exception.TargetSite, exception.Message));
                Write("  Stack trace:\n" + exception.StackTrace + "\n");
            }
        }

        private static void Write(string text)
        {
            try
            {
                logFile.Write(text);
            }
            catch (ObjectDisposedException)
            {
                // Deliberately do nothing. The application is exiting, and the log file is closed.
                // A lingering thread is probably trying to log something unimportant.
            }
        }
    }
}
