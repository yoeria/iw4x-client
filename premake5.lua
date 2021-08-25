gitVersioningCommand = "git describe --tags --dirty --always"
gitCurrentBranchCommand = "git symbolic-ref -q --short HEAD"

-- Quote the given string input as a C string
function cstrquote(value)
	result = value:gsub("\\", "\\\\")
	result = result:gsub("\"", "\\\"")
	result = result:gsub("\n", "\\n")
	result = result:gsub("\t", "\\t")
	result = result:gsub("\r", "\\r")
	result = result:gsub("\a", "\\a")
	result = result:gsub("\b", "\\b")
	result = "\"" .. result .. "\""
	return result
end

-- Converts tags in "vX.X.X" format to an array of numbers {X,X,X}.
-- In the case where the format does not work fall back to old {4,2,REVISION}.
function vertonumarr(value, vernumber)
	vernum = {}
	for num in string.gmatch(value, "%d+") do
		table.insert(vernum, tonumber(num))
	end
	if #vernum < 3 then
		return {4,2,tonumber(vernumber)}
	end
	return vernum
end

-- Option to allow copying the DLL file to a custom folder after build
newoption {
	trigger = "copy-to",
	description = "Optional, copy the DLL to a custom folder after build, define the path here if wanted.",
	value = "PATH"
}

newoption {
	trigger = "no-new-structure",
	description = "Do not use new virtual path structure (separating headers and source files)."
}

newoption {
	trigger = "copy-pdb",
	description = "Copy debug information for binaries as well to the path given via --copy-to."
}

newoption {
	trigger = "ac-disable",
	description = "Disable anticheat."
}

newoption {
	trigger = "ac-debug-detections",
	description = "Log anticheat detections."
}

newoption {
	trigger = "ac-debug-load-library",
	description = "Log libraries that get loaded."
}

newoption {
	trigger = "force-unit-tests",
	description = "Always compile unit tests."
}

newoption {
	trigger = "force-exception-handler",
	description = "Install custom unhandled exception handler even for Debug builds."
}

newoption {
	trigger = "force-minidump-upload",
	description = "Upload minidumps even for Debug builds."
}

newoption {
	trigger = "iw4x-zones",
	description = "Zonebuilder generates iw4x zones that cannot be loaded without IW4x specific patches."
}

newoption {
	trigger = "aimassist-enable",
	description = "Enables code for controller aim assist."
}

newaction {
	trigger = "version",
	description = "Returns the version string for the current commit of the source code.",
	onWorkspace = function(wks)
		-- get current version via git
		local proc = assert(io.popen(gitVersioningCommand, "r"))
		local gitDescribeOutput = assert(proc:read('*a')):gsub("%s+", "")
		proc:close()
		local version = gitDescribeOutput

		proc = assert(io.popen(gitCurrentBranchCommand, "r"))
		local gitCurrentBranchOutput = assert(proc:read('*a')):gsub("%s+", "")
		local gitCurrentBranchSuccess = proc:close()
		if gitCurrentBranchSuccess then
			-- We got a branch name, check if it is a feature branch
			if gitCurrentBranchOutput ~= "develop" and gitCurrentBranchOutput ~= "master" then
				version = version .. "-" .. gitCurrentBranchOutput
			end
		end

		print(version)
		os.exit(0)
	end
}

newaction {
	trigger = "generate-buildinfo",
	description = "Sets up build information file like version.h.",
	onWorkspace = function(wks)
		-- get revision number via git
		local proc = assert(io.popen("git rev-list --count HEAD", "r"))
		local revNumber = assert(proc:read('*a')):gsub("%s+", "")

		-- get current version via git
		local proc = assert(io.popen(gitVersioningCommand, "r"))
		local gitDescribeOutput = assert(proc:read('*a')):gsub("%s+", "")
		proc:close()

		-- get whether this is a clean revision (no uncommitted changes)
		proc = assert(io.popen("git status --porcelain", "r"))
		local revDirty = (assert(proc:read('*a')) ~= "")
		if revDirty then revDirty = 1 else revDirty = 0 end
		proc:close()

		-- get current tag name
		proc = assert(io.popen("git describe --tags --abbrev=0"))
		local tagName = assert(proc:read('*l'))

		-- get old version number from version.hpp if any
		local oldVersion = "(none)"
		local oldVersionHeader = io.open(wks.location .. "/src/version.h", "r")
		if oldVersionHeader ~= nil then
			local oldVersionHeaderContent = assert(oldVersionHeader:read('*l'))
			while oldVersionHeaderContent do
				m = string.match(oldVersionHeaderContent, "#define GIT_DESCRIBE (.+)%s*$")
				if m ~= nil then
					oldVersion = m
				end

				oldVersionHeaderContent = oldVersionHeader:read('*l')
			end
		end

		-- generate version.hpp with a revision number if not equal
		gitDescribeOutputQuoted = cstrquote(gitDescribeOutput)
		if oldVersion ~= gitDescribeOutputQuoted then
			print ("Update " .. oldVersion .. " -> " .. gitDescribeOutputQuoted)
			local versionHeader = assert(io.open(wks.location .. "/src/version.h", "w"))
			versionHeader:write("/*\n")
			versionHeader:write(" * Automatically generated by premake5.\n")
			versionHeader:write(" * Do not touch, you fucking moron!\n")
			versionHeader:write(" */\n")
			versionHeader:write("\n")
			versionHeader:write("#define GIT_DESCRIBE " .. gitDescribeOutputQuoted .. "\n")
			versionHeader:write("#define GIT_DIRTY " .. revDirty .. "\n")
			versionHeader:write("#define GIT_TAG " .. cstrquote(tagName) .. "\n")
			versionHeader:write("\n")
			versionHeader:write("// Legacy definitions (needed for update check)\n")
			versionHeader:write("#define REVISION " .. revNumber .. "\n")
			versionHeader:write("\n")
			versionHeader:write("// Version transformed for RC files\n")
			versionHeader:write("#define VERSION_RC " .. table.concat(vertonumarr(tagName, revNumber), ",") .. "\n")
			versionHeader:write("\n")
			versionHeader:write("// Alias definitions\n")
			versionHeader:write("#define VERSION GIT_DESCRIBE\n")
			versionHeader:write("#define SHORTVERSION " .. cstrquote(table.concat(vertonumarr(tagName, revNumber), ".")) .. "\n")
			versionHeader:close()
			local versionHeader = assert(io.open(wks.location .. "/src/version.hpp", "w"))
			versionHeader:write("/*\n")
			versionHeader:write(" * Automatically generated by premake5.\n")
			versionHeader:write(" * Do not touch, you fucking moron!\n")
			versionHeader:write(" *\n")
			versionHeader:write(" * This file exists for reasons of complying with our coding standards.\n")
			versionHeader:write(" *\n")
			versionHeader:write(" * The Resource Compiler will ignore any content from C++ header files if they're not from STDInclude.hpp.\n")
			versionHeader:write(" * That's the reason why we now place all version info in version.h instead.\n")
			versionHeader:write(" */\n")
			versionHeader:write("\n")
			versionHeader:write("#include \".\\version.h\"\n")
			versionHeader:close()
		end
	end
}

depsBasePath = "./deps"

require "premake/json11"
require "premake/libtomcrypt"
require "premake/libtommath"
require "premake/mongoose"
require "premake/pdcurses"
require "premake/protobuf"
require "premake/zlib"
require "premake/udis86"
require "premake/iw4mvm"
require "premake/dxsdk"

json11.setup
{
	source = path.join(depsBasePath, "json11"),
}
libtomcrypt.setup
{
	defines = {
		"LTC_NO_FAST",
		"LTC_NO_PROTOTYPES",
		"LTC_NO_RSA_BLINDING",
	},
	source = path.join(depsBasePath, "libtomcrypt"),
}
libtommath.setup
{
	defines = {
		"LTM_DESC",
		"__STDC_IEC_559__",
	},
	source = path.join(depsBasePath, "libtommath"),
}
mongoose.setup
{
	source = path.join(depsBasePath, "mongoose"),
}
pdcurses.setup
{
	source = path.join(depsBasePath, "pdcurses"),
}
protobuf.setup
{
	source = path.join(depsBasePath, "protobuf"),
}
zlib.setup
{
	defines = {
		"ZLIB_CONST"
	},
	source = path.join(depsBasePath, "zlib"),
}
udis86.setup
{
	source = path.join(depsBasePath, "udis86"),
}
iw4mvm.setup
{
	defines = {
		"IW4X",
		"DETOURS_X86",
		"DETOURS_32BIT",
	},
	source = path.join(depsBasePath, "iw4mvm"),
}
dxsdk.setup
{
	source = path.join(depsBasePath, "dxsdk"),
}

workspace "iw4x"
	startproject "iw4x"
	location "./build"
	objdir "%{wks.location}/obj"
	targetdir "%{wks.location}/bin/%{cfg.buildcfg}"
	buildlog "%{wks.location}/obj/%{cfg.architecture}/%{cfg.buildcfg}/%{prj.name}/%{prj.name}.log"
	configurations { "Debug", "Release" }
	architecture "x86"
	platforms "x86"
	--exceptionhandling ("SEH")

	staticruntime "On"

	configuration "windows"
		defines { "_WINDOWS", "WIN32" }

	configuration "Release*"
		defines { "NDEBUG" }
		flags { "MultiProcessorCompile", "LinkTimeOptimization", "No64BitChecks" }
		optimize "On"

		if not _OPTIONS["force-unit-tests"] then
			rtti ("Off")
		end

	configuration "Debug*"
		defines { "DEBUG", "_DEBUG" }
		flags { "MultiProcessorCompile", "No64BitChecks" }
		optimize "Debug"
		if symbols ~= nil then
			symbols "On"
		else
			flags { "Symbols" }
		end

	project "iw4x"
		kind "SharedLib"
		language "C++"
		files {
			"./src/**.rc",
			"./src/**.hpp",
			"./src/**.cpp",
			--"./src/**.proto",
		}
		includedirs {
			"%{prj.location}/src",
			"./src",
			"./lib/include",
		}
		syslibdirs {
			"./lib/bin",
		}
		resincludedirs {
			"$(ProjectDir)src" -- fix for VS IDE
		}

		-- Debug flags
		if _OPTIONS["ac-disable"] then
			defines { "DISABLE_ANTICHEAT" }
		end
		if _OPTIONS["ac-debug-detections"] then
			defines { "DEBUG_DETECTIONS" }
		end
		if _OPTIONS["ac-debug-load-library"] then
			defines { "DEBUG_LOAD_LIBRARY" }
		end
		if _OPTIONS["force-unit-tests"] then
			defines { "FORCE_UNIT_TESTS" }
		end
		if _OPTIONS["force-minidump-upload"] then
			defines { "FORCE_MINIDUMP_UPLOAD" }
		end
		if _OPTIONS["force-exception-handler"] then
			defines { "FORCE_EXCEPTION_HANDLER" }
		end
		if _OPTIONS["iw4x-zones"] then
			defines { "GENERATE_IW4X_SPECIFIC_ZONES" }
		end
		if _OPTIONS["aimassist-enable"] then
			defines { "AIM_ASSIST_ENABLED" }
		end

		-- Pre-compiled header
		pchheader "STDInclude.hpp" -- must be exactly same as used in #include directives
		pchsource "src/STDInclude.cpp" -- real path
		buildoptions { "/Zm200" }

		-- Dependency libraries
		json11.import()
		libtomcrypt.import()
		libtommath.import()
		mongoose.import()
		pdcurses.import()
		protobuf.import()
		zlib.import()
		udis86.import()
		--iw4mvm.import()
		dxsdk.import()

		-- fix vpaths for protobuf sources
		vpaths
		{
			["*"] = { "./src/**" },
			--["Proto/Generated"] = { "**.pb.*" }, -- meh.
		}

		-- Virtual paths
		if not _OPTIONS["no-new-structure"] then
			vpaths
			{
				["Headers/*"] = { "./src/**.hpp" },
				["Sources/*"] = { "./src/**.cpp" },
				["Resource/*"] = { "./src/**.rc" },
				--["Proto/Definitions/*"] = { "./src/Proto/**.proto" },
				--["Proto/Generated/*"] = { "**.pb.*" }, -- meh.
			}
		end

		vpaths
		{
			["Docs/*"] = { "**.txt","**.md" },
		}

		-- Pre-build
		prebuildcommands
		{
			"pushd %{_MAIN_SCRIPT_DIR}",
			"tools\\premake5 generate-buildinfo",
			"popd",
		}

		-- Post-build
		if _OPTIONS["copy-to"] then
			saneCopyToPath = string.gsub(_OPTIONS["copy-to"] .. "\\", "\\\\", "\\")
			postbuildcommands {
				"if not exist \"" .. saneCopyToPath .. "\" mkdir \"" .. saneCopyToPath .. "\"",
			}

			if _OPTIONS["copy-pdb"] then
				postbuildcommands {
					"copy /y \"$(TargetDir)*.pdb\" \"" .. saneCopyToPath .. "\"",
				}
			end

			-- This has to be the last one, as otherwise VisualStudio will succeed building even if copying fails
			postbuildcommands {
				"copy /y \"$(TargetDir)*.dll\" \"" .. saneCopyToPath .. "\"",
			}
		end

		-- Specific configurations
		flags { "UndefinedIdentifiers" }
		warnings "Extra"

		if symbols ~= nil then
			symbols "On"
		else
			flags { "Symbols" }
		end

		configuration "Release*"
			flags {
				"FatalCompileWarnings",
				"FatalLinkWarnings",
			}
		configuration {}

		--[[
		-- Generate source code from protobuf definitions
		rules { "ProtobufCompiler" }

		-- Workaround: Consume protobuf generated source files
		matches = os.matchfiles(path.join("src/Proto/**.proto"))
		for i, srcPath in ipairs(matches) do
			basename = path.getbasename(srcPath)
			files
			{
				string.format("%%{prj.location}/src/proto/%s.pb.h", basename),
				string.format("%%{prj.location}/src/proto/%s.pb.cc", basename),
			}
		end
		includedirs
		{
			"%{prj.location}/src/proto",
		}
		filter "files:**.pb.*"
			flags {
				"NoPCH",
			}
			buildoptions {
				"/wd4100", -- "Unused formal parameter"
				"/wd4389", -- "Signed/Unsigned mismatch"
				"/wd6011", -- "Dereferencing NULL pointer"
				"/wd4125", -- "Decimal digit terminates octal escape sequence"
			}
			defines {
				"_SCL_SECURE_NO_WARNINGS",
			}
		filter {}
		]]

	group "External dependencies"
		json11.project()
		libtomcrypt.project()
		libtommath.project()
		mongoose.project()
		pdcurses.project()
		protobuf.project()
		zlib.project()
		udis86.project()
		--iw4mvm.project()
		
workspace "*"
	cppdialect "C++17"
	defines { "_SILENCE_ALL_CXX17_DEPRECATION_WARNINGS" }

rule "ProtobufCompiler"
	display "Protobuf compiler"
	location "./build"
	fileExtension ".proto"
	buildmessage "Compiling %(Identity) with protoc..."
	buildcommands {
		'@echo off',
		'path "$(SolutionDir)\\..\\tools"',
		'if not exist "$(ProjectDir)\\src\\proto" mkdir "$(ProjectDir)\\src\\proto"',
		'protoc --error_format=msvs -I=%(RelativeDir) --cpp_out=src\\proto %(Identity)',
	}
	buildoutputs {
		'$(ProjectDir)\\src\\proto\\%(Filename).pb.cc',
		'$(ProjectDir)\\src\\proto\\%(Filename).pb.h',
	}
