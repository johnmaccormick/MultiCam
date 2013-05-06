using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Diagnostics;

namespace Multicam2
{
    static class KeyHooks
    {
        static string[] executable_names = { "Skype.exe", "PlayCap.exe" };


        public static void AddHooks()
        {
            foreach (var executable in executable_names)
            {
                Process[] processes = Process.GetProcessesByName(executable);
                foreach (var process in processes)
                {
                    ProcessThreadCollection threads = process.Threads;
                    foreach (ProcessThread thread in threads)
                    {
                        Console.WriteLine(thread.Id);
                    }
                }
            }
        }
    }
}
