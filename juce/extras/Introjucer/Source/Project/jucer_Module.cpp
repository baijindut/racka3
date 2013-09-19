/*
  ==============================================================================

   This file is part of the JUCE library.
   Copyright (c) 2013 - Raw Material Software Ltd.

   Permission is granted to use this software under the terms of either:
   a) the GPL v2 (or any later version)
   b) the Affero GPL v3

   Details of these licenses can be found at: www.gnu.org/licenses

   JUCE is distributed in the hope that it will be useful, but WITHOUT ANY
   WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
   A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

   ------------------------------------------------------------------------------

   To release a closed-source product which uses JUCE, commercial licenses are
   available: visit www.juce.com for more information.

  ==============================================================================
*/

#include "jucer_Module.h"
#include "jucer_ProjectType.h"
#include "../Project Saving/jucer_ProjectExporter.h"
#include "../Project Saving/jucer_ProjectSaver.h"
#include "jucer_AudioPluginModule.h"


//==============================================================================
AvailableModuleList::AvailableModuleList()
{
}

AvailableModuleList::AvailableModuleList (const AvailableModuleList& other)
    : moduleFolder (other.moduleFolder)
{
    modules.addCopiesOf (other.modules);
}

AvailableModuleList& AvailableModuleList::operator= (const AvailableModuleList& other)
{
    moduleFolder = other.moduleFolder;
    modules.clear();
    modules.addCopiesOf (other.modules);

    return *this;
}

bool AvailableModuleList::operator== (const AvailableModuleList& other) const
{
    if (modules.size() != other.modules.size())
        return false;

    for (int i = modules.size(); --i >= 0;)
    {
        const Module* m1 = modules.getUnchecked(i);
        const Module* m2 = other.findModuleInfo (m1->uid);

        if (m2 == nullptr || *m1 != *m2)
            return false;
    }

    return true;
}

bool AvailableModuleList::isLocalModulesFolderValid()
{
    return isModulesFolder (getModulesFolderForJuceOrModulesFolder (getLocalModulesFolder (nullptr)));
}

static int getVersionElement (const String& v, int index)
{
    StringArray parts;
    parts.addTokens (v, "., ", String::empty);

    return parts [parts.size() - index - 1].getIntValue();
}

static int getJuceVersion (const String& v)
{
    return getVersionElement (v, 2) * 100000
         + getVersionElement (v, 1) * 1000
         + getVersionElement (v, 0);
}

static int getBuiltJuceVersion()
{
    return JUCE_MAJOR_VERSION * 100000
         + JUCE_MINOR_VERSION * 1000
         + JUCE_BUILDNUMBER;
}

bool AvailableModuleList::isLibraryNewerThanIntrojucer()
{
    AvailableModuleList list;
    list.rescan (getModulesFolderForJuceOrModulesFolder (getLocalModulesFolder (nullptr)));

    for (int i = list.modules.size(); --i >= 0;)
    {
        const Module* m = list.modules.getUnchecked(i);

        if (m->uid.startsWith ("juce_")
             && getJuceVersion (m->version) > getBuiltJuceVersion())
            return true;
    }

    return false;
}

bool AvailableModuleList::isJuceFolder (const File& folder)
{
    return folder.getFileName().containsIgnoreCase ("juce")
             && isModulesFolder (folder.getChildFile ("modules"));
}

bool AvailableModuleList::isModulesFolder (const File& folder)
{
    return folder.getFileName().equalsIgnoreCase ("modules")
             && folder.isDirectory();
}

bool AvailableModuleList::isJuceOrModulesFolder (const File& folder)
{
    return isJuceFolder (folder) || isModulesFolder (folder);
}

File AvailableModuleList::getModulesFolderForJuceOrModulesFolder (const File& f)
{
    if (f.getFileName() != "modules" && f.isDirectory() && f.getChildFile ("modules").isDirectory())
        return f.getChildFile ("modules");

    return f;
}

File AvailableModuleList::getModulesFolderForExporter (const ProjectExporter& exporter)
{
    File f (exporter.getProject().resolveFilename (exporter.getJuceFolderString()));
    return getModulesFolderForJuceOrModulesFolder (f);
}

File AvailableModuleList::getDefaultModulesFolder (Project* project)
{
    if (project != nullptr)
    {
        for (Project::ExporterIterator exporter (*project); exporter.next();)
        {
            const File f (getModulesFolderForExporter (*exporter));

            if (AvailableModuleList::isModulesFolder (f))
                return f;
        }
    }

    // Fall back to a default..
   #if JUCE_WINDOWS
    return File::getSpecialLocation (File::userDocumentsDirectory)
   #else
    return File::getSpecialLocation (File::userHomeDirectory)
   #endif
            .getChildFile ("juce")
            .getChildFile ("modules");
}

File AvailableModuleList::getLocalModulesFolder (Project* project)
{
    File defaultJuceFolder (getDefaultModulesFolder (project));

    File f (getGlobalProperties().getValue ("lastJuceFolder", defaultJuceFolder.getFullPathName()));
    f = getModulesFolderForJuceOrModulesFolder (f);

    if ((! AvailableModuleList::isModulesFolder (f)) && AvailableModuleList::isModulesFolder (defaultJuceFolder))
        f = defaultJuceFolder;

    return f;
}

void AvailableModuleList::setLocalModulesFolder (const File& file)
{
    //jassert (FileHelpers::isJuceFolder (file));
    getGlobalProperties().setValue ("lastJuceFolder", file.getFullPathName());
}

struct ModuleSorter
{
    static int compareElements (const AvailableModuleList::Module* m1, const AvailableModuleList::Module* m2)
    {
        return m1->uid.compareIgnoreCase (m2->uid);
    }
};

void AvailableModuleList::sort()
{
    ModuleSorter sorter;
    modules.sort (sorter);
}

void AvailableModuleList::rescan()
{
    rescan (moduleFolder);
}

Result AvailableModuleList::rescan (const File& newModulesFolder)
{
    modules.clear();
    moduleFolder = getModulesFolderForJuceOrModulesFolder (newModulesFolder);

    if (moduleFolder.isDirectory())
    {
        DirectoryIterator iter (moduleFolder, false, "*", File::findDirectories);

        while (iter.next())
        {
            const File moduleDef (iter.getFile().getLinkedTarget()
                                    .getChildFile (LibraryModule::getInfoFileName()));

            if (moduleDef.exists())
            {
                LibraryModule m (moduleDef);

                if (! m.isValid())
                    return Result::fail ("Failed to load module manifest: " + moduleDef.getFullPathName());

                Module* info = new Module();
                modules.add (info);

                info->uid = m.getID();
                info->version = m.getVersion();
                info->name = m.moduleInfo ["name"];
                info->description = m.moduleInfo ["description"];
                info->license = m.moduleInfo ["license"];
                info->file = moduleDef;
            }
        }
    }

    sort();
    return Result::ok();
}

bool AvailableModuleList::loadFromWebsite()
{
    modules.clear();

    URL baseURL ("http://www.juce.com/juce/modules");
    URL url (baseURL.getChildURL ("modulelist.php"));

    var infoList (JSON::parse (url.readEntireTextStream (false)));

    if (infoList.isArray())
    {
        const Array<var>* moduleList = infoList.getArray();

        for (int i = 0; i < moduleList->size(); ++i)
        {
            const var& m = moduleList->getReference(i);
            const String file (m ["file"].toString());

            if (file.isNotEmpty())
            {
                var moduleInfo (m ["info"]);
                LibraryModule lm (moduleInfo);

                if (lm.isValid())
                {
                    Module* info = new Module();
                    modules.add (info);

                    info->uid = lm.getID();
                    info->version = lm.getVersion();
                    info->name = lm.getName();
                    info->description = lm.getDescription();
                    info->license = lm.getLicense();
                    info->url = baseURL.getChildURL (file);
                }
            }
        }
    }

    sort();
    return infoList.isArray();
}

LibraryModule* AvailableModuleList::Module::create() const
{
    return new LibraryModule (file);
}

bool AvailableModuleList::Module::operator== (const Module& other) const
{
    return uid == other.uid
             && version == other.version
             && name == other.name
             && description == other.description
             && license == other.license
             && file == other.file
             && url == other.url;
}

bool AvailableModuleList::Module::operator!= (const Module& other) const
{
    return ! operator== (other);
}

LibraryModule* AvailableModuleList::loadModule (const String& uid) const
{
    if (const Module* const m = findModuleInfo (uid))
        return m->create();

    return nullptr;
}

const AvailableModuleList::Module* AvailableModuleList::findModuleInfo (const String& uid) const
{
    for (int i = modules.size(); --i >= 0;)
        if (modules.getUnchecked(i)->uid == uid)
            return modules.getUnchecked(i);

    return nullptr;
}

void AvailableModuleList::getDependencies (const String& moduleID, StringArray& dependencies) const
{
    ScopedPointer<LibraryModule> m (loadModule (moduleID));

    if (m != nullptr)
    {
        const var depsArray (m->moduleInfo ["dependencies"]);

        if (const Array<var>* const deps = depsArray.getArray())
        {
            for (int i = 0; i < deps->size(); ++i)
            {
                const var& d = deps->getReference(i);

                String uid (d ["id"].toString());
                String version (d ["version"].toString());

                if (! dependencies.contains (uid, true))
                {
                    dependencies.add (uid);
                    getDependencies (uid, dependencies);
                }
            }
        }
    }
}

void AvailableModuleList::createDependencies (const String& moduleID, OwnedArray<LibraryModule>&) const
{
    ScopedPointer<LibraryModule> m (loadModule (moduleID));

    if (m != nullptr)
    {
        const var depsArray (m->moduleInfo ["dependencies"]);

        if (const Array<var>* const deps = depsArray.getArray())
        {
            for (int i = 0; i < deps->size(); ++i)
            {
                const var& d = deps->getReference(i);

                String uid (d ["id"].toString());
                String version (d ["version"].toString());

                //xxx to do - also need to find version conflicts
                jassertfalse;
            }
        }
    }
}

StringArray AvailableModuleList::getExtraDependenciesNeeded (Project& project, const AvailableModuleList::Module& m)
{
    StringArray dependencies, extraDepsNeeded;
    getDependencies (m.uid, dependencies);

    for (int i = 0; i < dependencies.size(); ++i)
        if ((! project.getModules().isModuleEnabled (dependencies[i])) && dependencies[i] != m.uid)
            extraDepsNeeded.add (dependencies[i]);

    return extraDepsNeeded;
}

//==============================================================================
LibraryModule::LibraryModule (const File& file)
    : moduleInfo (JSON::parse (file)),
      moduleFile (file),
      moduleFolder (file.getParentDirectory())
{
}

LibraryModule::LibraryModule (const var& info)
    : moduleInfo (info)
{
}

bool LibraryModule::isValid() const         { return getID().isNotEmpty(); }

bool LibraryModule::isPluginClient() const                          { return getID() == "juce_audio_plugin_client"; }
bool LibraryModule::isAUPluginHost (const Project& project) const   { return getID() == "juce_audio_processors" && project.isConfigFlagEnabled ("JUCE_PLUGINHOST_AU"); }
bool LibraryModule::isVSTPluginHost (const Project& project) const  { return getID() == "juce_audio_processors" && project.isConfigFlagEnabled ("JUCE_PLUGINHOST_VST"); }

File LibraryModule::getInclude (const File& folder) const
{
    return folder.getChildFile (moduleInfo ["include"].toString());
}

RelativePath LibraryModule::getModuleRelativeToProject (ProjectExporter& exporter) const
{
    RelativePath p (exporter.getJuceFolderString(), RelativePath::projectFolder);
    if (p.getFileName() != "modules")
        p = p.getChildFile ("modules");

    return p.getChildFile (getID());
}

RelativePath LibraryModule::getModuleOrLocalCopyRelativeToProject (ProjectExporter& exporter, const File& localModuleFolder) const
{
    if (exporter.getProject().getModules().shouldCopyModuleFilesLocally (getID()).getValue())
        return RelativePath (exporter.getProject().getRelativePathForFile (localModuleFolder), RelativePath::projectFolder);

    return getModuleRelativeToProject (exporter);
}

//==============================================================================
void LibraryModule::writeIncludes (ProjectSaver& projectSaver, OutputStream& out)
{
    const File localModuleFolder (projectSaver.getLocalModuleFolder (*this));
    const File localHeader (getInclude (localModuleFolder));

    if (projectSaver.getProject().getModules().shouldCopyModuleFilesLocally (getID()).getValue())
    {
        projectSaver.copyFolder (moduleFolder, localModuleFolder);
    }
    else
    {
        localModuleFolder.createDirectory();
        createLocalHeaderWrapper (projectSaver, getInclude (moduleFolder), localHeader);
    }

    out << CodeHelpers::createIncludeStatement (localHeader, projectSaver.getGeneratedCodeFolder().getChildFile ("AppConfig.h")) << newLine;
}

static void writeGuardedInclude (OutputStream& out, StringArray paths, StringArray guards)
{
    StringArray uniquePaths (paths);
    uniquePaths.removeDuplicates (false);

    if (uniquePaths.size() == 1)
    {
        out << "#include " << paths[0] << newLine;
    }
    else
    {
        for (int i = paths.size(); --i >= 0;)
        {
            for (int j = i; --j >= 0;)
            {
                if (paths[i] == paths[j] && guards[i] == guards[j])
                {
                    paths.remove (i);
                    guards.remove (i);
                }
            }
        }

        for (int i = 0; i < paths.size(); ++i)
        {
            out << (i == 0 ? "#if " : "#elif ") << guards[i] << newLine
                << " #include " << paths[i] << newLine;
        }

        out << "#else" << newLine
            << " #error \"This file is designed to be used in an Introjucer-generated project!\"" << newLine
            << "#endif" << newLine;
    }
}

void LibraryModule::createLocalHeaderWrapper (ProjectSaver& projectSaver, const File& originalHeader, const File& localHeader) const
{
    Project& project = projectSaver.getProject();

    MemoryOutputStream out;

    out << "// This is an auto-generated file to redirect any included" << newLine
        << "// module headers to the correct external folder." << newLine
        << newLine;

    StringArray paths, guards;

    for (Project::ExporterIterator exporter (project); exporter.next();)
    {
        const RelativePath headerFromProject (getModuleRelativeToProject (*exporter)
                                                .getChildFile (originalHeader.getFileName()));

        const RelativePath fileFromHere (headerFromProject.rebased (project.getFile().getParentDirectory(),
                                                                    localHeader.getParentDirectory(), RelativePath::unknown));

        paths.add (fileFromHere.toUnixStyle().quoted());
        guards.add ("defined (" + exporter->getExporterIdentifierMacro() + ")");
    }

    writeGuardedInclude (out, paths, guards);
    out << newLine;

    projectSaver.replaceFileIfDifferent (localHeader, out);
}

//==============================================================================
File LibraryModule::getLocalFolderFor (Project& project) const
{
    if (project.getModules().shouldCopyModuleFilesLocally (getID()).getValue())
        return project.getGeneratedCodeFolder().getChildFile ("modules").getChildFile (getID());

    return moduleFolder;
}

void LibraryModule::prepareExporter (ProjectExporter& exporter, ProjectSaver& projectSaver) const
{
    Project& project = exporter.getProject();

    File localFolder (moduleFolder);
    if (project.getModules().shouldCopyModuleFilesLocally (getID()).getValue())
        localFolder = projectSaver.getLocalModuleFolder (*this);

    {
        Array<File> compiled;
        findAndAddCompiledCode (exporter, projectSaver, localFolder, compiled);

        if (project.getModules().shouldShowAllModuleFilesInProject (getID()).getValue())
            addBrowsableCode (exporter, compiled, localFolder);
    }

    if (isVSTPluginHost (project))
        VSTHelpers::addVSTFolderToPath (exporter, exporter.extraSearchPaths);

    if (exporter.isXcode())
    {
        if (isAUPluginHost (project))
            exporter.xcodeFrameworks.addTokens ("AudioUnit CoreAudioKit", false);

        const String frameworks (moduleInfo [exporter.isOSX() ? "OSXFrameworks" : "iOSFrameworks"].toString());
        exporter.xcodeFrameworks.addTokens (frameworks, ", ", String::empty);
    }
    else if (exporter.isLinux())
    {
        const String libs (moduleInfo ["LinuxLibs"].toString());
        exporter.linuxLibs.addTokens (libs, ", ", String::empty);
        exporter.linuxLibs.trim();
        exporter.linuxLibs.sort (false);
        exporter.linuxLibs.removeDuplicates (false);
    }
    else if (exporter.isCodeBlocks())
    {
        const String libs (moduleInfo ["mingwLibs"].toString());
        exporter.mingwLibs.addTokens (libs, ", ", String::empty);
        exporter.mingwLibs.trim();
        exporter.mingwLibs.sort (false);
        exporter.mingwLibs.removeDuplicates (false);
    }

    if (isPluginClient())
    {
        if (shouldBuildVST  (project).getValue())  VSTHelpers::prepareExporter (exporter, projectSaver);
        if (shouldBuildAU   (project).getValue())  AUHelpers::prepareExporter (exporter, projectSaver);
        if (shouldBuildAAX  (project).getValue())  AAXHelpers::prepareExporter (exporter, projectSaver, localFolder);
        if (shouldBuildRTAS (project).getValue())  RTASHelpers::prepareExporter (exporter, projectSaver, localFolder);
    }
}

void LibraryModule::createPropertyEditors (ProjectExporter& exporter, PropertyListBuilder& props) const
{
    if (isVSTPluginHost (exporter.getProject())
         && ! (isPluginClient() && shouldBuildVST  (exporter.getProject()).getValue()))
        VSTHelpers::createVSTPathEditor (exporter, props);

    if (isPluginClient())
    {
        if (shouldBuildVST  (exporter.getProject()).getValue())  VSTHelpers::createPropertyEditors (exporter, props);
        if (shouldBuildRTAS (exporter.getProject()).getValue())  RTASHelpers::createPropertyEditors (exporter, props);
        if (shouldBuildAAX  (exporter.getProject()).getValue())  AAXHelpers::createPropertyEditors (exporter, props);
    }
}

void LibraryModule::getConfigFlags (Project& project, OwnedArray<Project::ConfigFlag>& flags) const
{
    const File header (getInclude (moduleFolder));
    jassert (header.exists());

    StringArray lines;
    header.readLines (lines);

    for (int i = 0; i < lines.size(); ++i)
    {
        String line (lines[i].trim());

        if (line.startsWith ("/**") && line.containsIgnoreCase ("Config:"))
        {
            ScopedPointer <Project::ConfigFlag> config (new Project::ConfigFlag());
            config->sourceModuleID = getID();
            config->symbol = line.fromFirstOccurrenceOf (":", false, false).trim();

            if (config->symbol.length() > 2)
            {
                ++i;

                while (! (lines[i].contains ("*/") || lines[i].contains ("@see")))
                {
                    if (lines[i].trim().isNotEmpty())
                        config->description = config->description.trim() + " " + lines[i].trim();

                    ++i;
                }

                config->description = config->description.upToFirstOccurrenceOf ("*/", false, false);
                config->value.referTo (project.getConfigFlag (config->symbol));
                flags.add (config.release());
            }
        }
    }
}

//==============================================================================
static bool exporterTargetMatches (const String& test, String target)
{
    StringArray validTargets;
    validTargets.addTokens (target, ",;", "");
    validTargets.trim();
    validTargets.removeEmptyStrings();

    if (validTargets.size() == 0)
        return true;

    for (int i = validTargets.size(); --i >= 0;)
    {
        const String& targetName = validTargets[i];

        if (targetName == test
             || (targetName.startsWithChar ('!') && test != targetName.substring (1).trimStart()))
            return true;
    }

    return false;
}

bool LibraryModule::fileTargetMatches (ProjectExporter& exporter, const String& target)
{
    if (exporter.isXcode())         return exporterTargetMatches ("xcode", target);
    if (exporter.isWindows())       return exporterTargetMatches ("msvc", target);
    if (exporter.isLinux())         return exporterTargetMatches ("linux", target);
    if (exporter.isAndroid())       return exporterTargetMatches ("android", target);
    if (exporter.isCodeBlocks())    return exporterTargetMatches ("mingw", target);
    return target.isEmpty();
}

void LibraryModule::findWildcardMatches (const File& localModuleFolder, const String& wildcardPath, Array<File>& result) const
{
    String path (wildcardPath.upToLastOccurrenceOf ("/", false, false));
    String wildCard (wildcardPath.fromLastOccurrenceOf ("/", false, false));

    Array<File> tempList;
    FileSorter sorter;

    DirectoryIterator iter (localModuleFolder.getChildFile (path), false, wildCard);
    bool isHiddenFile;

    while (iter.next (nullptr, &isHiddenFile, nullptr, nullptr, nullptr, nullptr))
        if (! isHiddenFile)
            tempList.addSorted (sorter, iter.getFile());

    result.addArray (tempList);
}

void LibraryModule::findAndAddCompiledCode (ProjectExporter& exporter, ProjectSaver& projectSaver,
                                            const File& localModuleFolder, Array<File>& result) const
{
    const var compileArray (moduleInfo ["compile"]); // careful to keep this alive while the array is in use!

    if (const Array<var>* const files = compileArray.getArray())
    {
        for (int i = 0; i < files->size(); ++i)
        {
            const var& file = files->getReference(i);
            const String filename (file ["file"].toString());

            if (filename.isNotEmpty()
                 && fileTargetMatches (exporter, file ["target"].toString()))
            {
                const File compiledFile (localModuleFolder.getChildFile (filename));
                result.add (compiledFile);

                Project::Item item (projectSaver.addFileToGeneratedGroup (compiledFile));

                if (file ["warnings"].toString().equalsIgnoreCase ("disabled"))
                    item.getShouldInhibitWarningsValue() = true;

                if (file ["stdcall"])
                    item.getShouldUseStdCallValue() = true;
            }
        }
    }
}

void LibraryModule::getLocalCompiledFiles (const File& localModuleFolder, Array<File>& result) const
{
    const var compileArray (moduleInfo ["compile"]); // careful to keep this alive while the array is in use!

    if (const Array<var>* const files = compileArray.getArray())
    {
        for (int i = 0; i < files->size(); ++i)
        {
            const var& file = files->getReference(i);
            const String filename (file ["file"].toString());

            if (filename.isNotEmpty()
                  #if JUCE_MAC
                   && exporterTargetMatches ("xcode", file ["target"].toString())
                  #elif JUCE_WINDOWS
                   && exporterTargetMatches ("msvc",  file ["target"].toString())
                  #elif JUCE_LINUX
                   && exporterTargetMatches ("linux", file ["target"].toString())
                  #endif
                )
            {
                result.add (localModuleFolder.getChildFile (filename));
            }
        }
    }
}

static void addFileWithGroups (Project::Item& group, const RelativePath& file, const String& path)
{
    const int slash = path.indexOfChar (File::separator);

    if (slash >= 0)
    {
        const String topLevelGroup (path.substring (0, slash));
        const String remainingPath (path.substring (slash + 1));

        Project::Item newGroup (group.getOrCreateSubGroup (topLevelGroup));
        addFileWithGroups (newGroup, file, remainingPath);
    }
    else
    {
        if (! group.containsChildForFile (file))
            group.addRelativeFile (file, -1, false);
    }
}

void LibraryModule::findBrowseableFiles (const File& localModuleFolder, Array<File>& filesFound) const
{
    const var filesArray (moduleInfo ["browse"]);

    if (const Array<var>* const files = filesArray.getArray())
        for (int i = 0; i < files->size(); ++i)
            findWildcardMatches (localModuleFolder, files->getReference(i), filesFound);
}

void LibraryModule::addBrowsableCode (ProjectExporter& exporter, const Array<File>& compiled, const File& localModuleFolder) const
{
    if (sourceFiles.size() == 0)
        findBrowseableFiles (localModuleFolder, sourceFiles);

    Project::Item sourceGroup (Project::Item::createGroup (exporter.getProject(), getID(), "__mainsourcegroup" + getID()));

    const RelativePath moduleFromProject (getModuleOrLocalCopyRelativeToProject (exporter, localModuleFolder));

    for (int i = 0; i < sourceFiles.size(); ++i)
    {
        const String pathWithinModule (FileHelpers::getRelativePathFrom (sourceFiles.getReference(i), localModuleFolder));

        // (Note: in exporters like MSVC we have to avoid adding the same file twice, even if one of those instances
        // is flagged as being excluded from the build, because this overrides the other and it fails to compile)
        if (exporter.canCopeWithDuplicateFiles() || ! compiled.contains (sourceFiles.getReference(i)))
            addFileWithGroups (sourceGroup,
                               moduleFromProject.getChildFile (pathWithinModule),
                               pathWithinModule);
    }

    sourceGroup.addFile (localModuleFolder.getChildFile (FileHelpers::getRelativePathFrom (moduleFile, moduleFolder)), -1, false);
    sourceGroup.addFile (getInclude (localModuleFolder), -1, false);

    exporter.getModulesGroup().state.addChild (sourceGroup.state.createCopy(), -1, nullptr);
}


//==============================================================================
EnabledModuleList::EnabledModuleList (Project& p, const ValueTree& s)
    : project (p), state (s)
{
}

EnabledModuleList::EnabledModuleList (const EnabledModuleList& other)
    : project (other.project), state (other.state)
{
}

const Identifier EnabledModuleList::modulesGroupTag ("MODULES");
const Identifier EnabledModuleList::moduleTag ("MODULE");

void EnabledModuleList::addDefaultModules (bool shouldCopyFilesLocally)
{
    const char* mods[] =
    {
        "juce_core",
        "juce_events",
        "juce_graphics",
        "juce_data_structures",
        "juce_gui_basics",
        "juce_gui_extra",
        "juce_gui_audio",
        "juce_cryptography",
        "juce_video",
        "juce_opengl",
        "juce_audio_basics",
        "juce_audio_devices",
        "juce_audio_formats",
        "juce_audio_processors"
    };

    for (int i = 0; i < numElementsInArray (mods); ++i)
        addModule (mods[i], shouldCopyFilesLocally);
}

bool EnabledModuleList::isModuleEnabled (const String& moduleID) const
{
    for (int i = 0; i < state.getNumChildren(); ++i)
        if (state.getChild(i) [Ids::ID] == moduleID)
            return true;

    return false;
}

bool EnabledModuleList::isAudioPluginModuleMissing() const
{
    return project.getProjectType().isAudioPlugin()
            && ! isModuleEnabled ("juce_audio_plugin_client");
}

Value EnabledModuleList::shouldShowAllModuleFilesInProject (const String& moduleID)
{
    return state.getChildWithProperty (Ids::ID, moduleID)
                .getPropertyAsValue (Ids::showAllCode, getUndoManager());
}

Value EnabledModuleList::shouldCopyModuleFilesLocally (const String& moduleID)
{
    return state.getChildWithProperty (Ids::ID, moduleID)
                .getPropertyAsValue (Ids::useLocalCopy, getUndoManager());
}

void EnabledModuleList::addModule (const String& moduleID, bool shouldCopyFilesLocally)
{
    if (! isModuleEnabled (moduleID))
    {
        ValueTree module (moduleTag);
        module.setProperty (Ids::ID, moduleID, nullptr);

        state.addChild (module, -1, getUndoManager());

        shouldShowAllModuleFilesInProject (moduleID) = true;
    }

    if (shouldCopyFilesLocally)
        shouldCopyModuleFilesLocally (moduleID) = true;
}

void EnabledModuleList::removeModule (const String& moduleID)
{
    for (int i = 0; i < state.getNumChildren(); ++i)
        if (state.getChild(i) [Ids::ID] == moduleID)
            state.removeChild (i, getUndoManager());
}

void EnabledModuleList::createRequiredModules (const AvailableModuleList& availableModules,
                                               OwnedArray<LibraryModule>& modules) const
{
    for (int i = 0; i < availableModules.modules.size(); ++i)
        if (isModuleEnabled (availableModules.modules.getUnchecked(i)->uid))
            modules.add (availableModules.modules.getUnchecked(i)->create());
}
