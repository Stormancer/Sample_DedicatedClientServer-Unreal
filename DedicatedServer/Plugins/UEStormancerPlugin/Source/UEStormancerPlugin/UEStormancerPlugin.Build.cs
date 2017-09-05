// Some copyright should be here...

using System;
using System.IO;
using System.Text.RegularExpressions;
using UnrealBuildTool;
using System.Diagnostics;
using System.Collections.Generic;

namespace UnrealBuildTool.Rules
{

    struct SetupInfo
    {
        public string _DownloadPath;
        public string _Version;
        public string _FileName;
        public string _InstallPath;

        public SetupInfo(string DownloadPath, string Version, string FileName, string InstallPath)
        {
            _DownloadPath = DownloadPath;
            _Version = Version;
            _FileName = FileName;
            _InstallPath = InstallPath;
        }
    }

    public class UEStormancerPlugin : ModuleRules
    {
        Dictionary<string, SetupInfo> SetupInfoMap = new Dictionary<string, SetupInfo>();

        private string VS_TOOLSET = "140";

        private string SDK_LIB_PATH = "";
        private string SDK_HEADER_PATH = "";
        private string SDK_DOWNLOAD_PATH = "https://github.com/Stormancer/stormancer-sdk-cpp/releases/download";
        private string SDK_VERSION = "v1.2";
        private string SDK_FILENAME = "Stormancer-cpp-1.2.zip";
        private string SDK_INSTALL_PATH = "";

        private string DCS_LIB_PATH = "";
        private string DCS_HEADER_PATH = "";
        private string DCS_DOWNLOAD_PATH = "https://github.com/Stormancer/Sample_DedicatedClientServer/releases/download";
        private string DCS_VERSION = "1.0";
        private string DCS_FILENAME = "Stormancer_DCS_1.0.zip";
        private string DCS_INSTALL_PATH = "";

        public UEStormancerPlugin(ReadOnlyTargetRules Target) : base(Target)
	    {
            SDK_LIB_PATH = ModuleDirectory + "/../../Resources/Stormancer/libs/";
            SDK_HEADER_PATH = ModuleDirectory + "/../../Resources/Stormancer/include/";
            SDK_INSTALL_PATH = ModuleDirectory + "/../../Resources/Stormancer";
            SetupInfoMap.Add("Stormancer", new SetupInfo(SDK_DOWNLOAD_PATH, SDK_VERSION, SDK_FILENAME, SDK_INSTALL_PATH));

            DCS_LIB_PATH = ModuleDirectory + "/../../Resources/DCS/Libs/";
            DCS_HEADER_PATH = ModuleDirectory + "/../../Resources/DCS/Public/";
            DCS_INSTALL_PATH = ModuleDirectory + "/../../Resources/DCS";
            SetupInfoMap.Add("DCS_OnlineModule", new SetupInfo(DCS_DOWNLOAD_PATH, DCS_VERSION, DCS_FILENAME, DCS_INSTALL_PATH));

            LoadLibrary(Target, DCS_HEADER_PATH, DCS_LIB_PATH, "DCS_OnlineModule");

            LoadLibrary(Target, SDK_HEADER_PATH, SDK_LIB_PATH, "Stormancer");
          
            SetupLocal(Target);
	    }

        /// <summary>
        /// Setup file in this plugin
        /// </summary>
        /// <param name="target">ReadOnlyTargetRules</param>
        private void SetupLocal(ReadOnlyTargetRules target)
        {
            PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[] {
                    "Source/UEStormancerPlugin/Public",
                }
                );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "Source/UEStormancerPlugin/Private",	
			    }
                );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
                    "Core",
			    }
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "CoreUObject",
                    "Engine",
                    "Slate",
                    "SlateCore",
                    "DedicatedServer", // Game module used to access game function from plugin.
			     }
                );

            DynamicallyLoadedModuleNames.AddRange(new string[]{});

        }


        /// <summary>
        /// Helper to setup an arbitrary library in the given library folder
        /// </summary>
        /// <param name="target">ReadOnlyTargetRules object</param>
        /// <param name="include_path">include_path Relative include path, eg. 3rdparty/mylib/include</param>
        /// <param name="build_path">build_path Relative build path, eg. 3rdparty/mylib/build</param>
        /// <param name="library_name">library_name Short library name, eg. mylib. Automatically expands to libmylib.a, mylib.lib, etc.</param>
        private void LoadLibrary(ReadOnlyTargetRules target, string include_path, string build_path, string library_name)
        {

            // Add the include path
            var full_include_path = Path.Combine(PluginPath, include_path);
            if (!Directory.Exists(full_include_path))
            {
                //Fail("Invalid include path: " + full_include_path);
                Trace("Missing include files at : {0}", full_include_path);
                DownloadResources(SetupInfoMap[library_name]);

            }
            else
            {
                SetupInfo info = SetupInfoMap[library_name];
                string versionFile = info._InstallPath + "\\" + info._FileName.Substring(0, info._FileName.Length-4);
                if (!File.Exists(versionFile))
                {
                    //File exists but it's not up-to-date
                    DownloadResources(SetupInfoMap[library_name]);
                }
                PublicIncludePaths.Add(full_include_path);
                Trace("Added include path: {0}", full_include_path);
            } 

            // Get the build path
            var full_build_path = Path.Combine(PluginPath, build_path);
            if (!Directory.Exists(full_build_path))
            {
                Fail("Invalid build path: " + full_build_path + " (Did you build the 3rdparty module already?)");
            }

            string platform = "";
            string extension = "lib";
            switch (target.Platform)
            {
                case UnrealTargetPlatform.Win64:
                    platform = "x64";
                    break;
                case UnrealTargetPlatform.Win32:
                    platform = "x86";
                    break;                
            }

            string configuration = "_Release_";
            //switch (target.Configuration)
            //{
            //    case UnrealTargetConfiguration.Debug:
            //        configuration = "_Debug_";
            //        break;
            //    case UnrealTargetConfiguration.DebugGame:
            //        //DebugUE4 is only needed on Windows
            //        if (target.Platform == UnrealTargetPlatform.Win64 || target.Platform == UnrealTargetPlatform.Win32)
            //        {
            //            configuration = "_DebugUE4_";
            //        }
            //        else
            //        {
            //            configuration = "_Debug_";
            //        }
            //        break;
            //    default:
            //        configuration = "_Release_";
            //        break;
            //}

            // Look at all the files in the build path; we need to smartly locate
            // the static library based on the current platform. For dynamic libraries
            // this is more difficult, but for static libraries, it's just .lib or .a
            string[] fileEntries = Directory.GetFiles(full_build_path);

            var pattern = ".*" + library_name + VS_TOOLSET + configuration + platform + "." + extension;

            Regex r = new Regex(pattern, RegexOptions.IgnoreCase);
            string full_library_path = null;
            foreach (var file in fileEntries)
            {
                if (r.Match(file).Success)
                {
                    full_library_path = Path.Combine(full_build_path, file);

                    break;
                }
            }
            if (full_library_path == null)
            {
                Fail("Unable to locate any build libraries in: " + full_build_path);
            }

            // Found a library; add it to the dependencies list
            PublicAdditionalLibraries.Add(full_library_path);
            Trace("Added static library: {0}", full_library_path);
        }

        /**
          * Print out a build message
           * Why error? Well, the UE masks all other errors. *shrug*
        */

        private void Trace(string msg)
        {
            Log.TraceError(Plugin + ": " + msg);
        }

        /// <summary>
        ///  Trace helper
        /// </summary>
        /// <param name="format"></param>
        /// <param name="args"></param>
        private void Trace(string format, params object[] args)
        {
            Trace(string.Format(format, args));
        }

        /// <summary>
        /// Raise an error
        /// </summary>
        /// <param name="message"></param>
        private void Fail(string message)
        {
            Trace(message);
            throw new Exception(message);
        }

        /// <summary>
        /// Get the absolute root to the plugin folder
        /// </summary>
        private string PluginPath
        {
            get
            {
                return Path.GetFullPath(Path.Combine(ModuleDirectory, "../.."));
            }
        }

        /// <summary>
        /// Get the name of this plugin's folder
        /// </summary>
        private string Plugin
        {        
            get
            {
                return new DirectoryInfo(PluginPath).Name;
            }
        }

        void DownloadResources(SetupInfo info)
        {
            ProcessStartInfo startInfo = new ProcessStartInfo(); // Maybe wrong I Think this is not launching a right powershell exe as I an launch it correctly with the arguments
            startInfo.FileName = "Powershell.exe";
            startInfo.Arguments = ModuleDirectory + "/../../SetupResources.ps1 -DownloadPath " + info._DownloadPath + " -Version " + info._Version + " -FileName " + info._FileName + " -InstallPath " + info._InstallPath;
     
            startInfo.RedirectStandardOutput = true;
            startInfo.RedirectStandardError = true;
            startInfo.UseShellExecute = false;
            startInfo.CreateNoWindow = true;
            Process process = new Process();
            process.StartInfo = startInfo;

            process.OutputDataReceived += (s, e) => Trace(e.Data);
            process.ErrorDataReceived += (s, e) => Trace(e.Data);
            process.Start();
            process.BeginOutputReadLine();
            process.BeginErrorReadLine();

            process.WaitForExit();
        }
    }

}

