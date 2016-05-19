/*
  ==============================================================================

    This file was auto-generated by the Introjucer!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#include "Precompiled.h"
#include "TinyRNN.h"
#include "MainComponent.h"
#include "PugiXMLSerializer.h"
#include "TrainingPipeline.h"
#include "BatchMidiProcessor.h"
#include "BatchTextProcessor.h"

class ScopedTimer final
{
public:
    
    explicit ScopedTimer(const std::string &targetName) :
    startTime(std::chrono::high_resolution_clock::now())
    {
        std::cout << targetName << std::endl;
    }
    
    ~ScopedTimer()
    {
        const auto endTime = std::chrono::high_resolution_clock::now();
        const auto milliSeconds = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - this->startTime).count();
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(endTime - this->startTime).count();
        
        if (seconds > 1)
        {
            std::cout << "Done (" << std::to_string(seconds) << " sec)" << std::endl;
        }
        else
        {
            std::cout << "Done (" << std::to_string(milliSeconds) << " ms)" << std::endl;
        }
    }
    
private:
    
    std::chrono::high_resolution_clock::time_point startTime;
    
    ScopedTimer(const ScopedTimer&) = delete;
    ScopedTimer &operator =(const ScopedTimer &) = delete;
};

class GoDeeperApplication : public JUCEApplication
{
public:
    
    GoDeeperApplication() {}
    
    virtual const String getApplicationName() override
    { return ProjectInfo::projectName; }
    
    virtual const String getApplicationVersion() override
    { return ProjectInfo::versionString; }
    
    virtual bool moreThanOneInstanceAllowed() override
    { return true; }
    
    virtual void initialise(const String &commandLine) override
    {
        static const String topologyFileName = "Topology.xml";
        static const String contextFileName = "Context.xml";
        static const String kernelsFileName = "Kernels.xml";
        static const String mappingFileName = "Mapping.xml";
        static const String latestDumpFileName = "LatestDump.xml";
        
        const String defaultCommandline = "init GoDeeper 40 120 120 40";
        //const String defaultCommandline = "text Deeper targets=Training";
        
        // need to filter out the xcode's garbage:
        const bool needsDefaultCommandline =
        (commandLine.isEmpty() || commandLine == "-NSDocumentRevisionsDebugMode YES");
        
        StringArray args =
        StringArray::fromTokens(needsDefaultCommandline ? defaultCommandline : commandLine, " =", "'\"");
        
        // usage:
        // init "MyLSTM" 256 128 128 128 256
        // train "MyLSTM" targets="Targets"
        
        if (args[0].toLowerCase() == "init")
        {
            const File networkDirectory(File::getCurrentWorkingDirectory().getChildFile(args[1].unquoted()));
            
            if (! networkDirectory.createDirectory().wasOk())
            {
                std::cout << "Failed to create network folder: " << networkDirectory.getFullPathName() << std::endl;
                this->quit();
                return;
            }
            
            // Estimated text generators:
            //
            // (35, {512, 256, 512}, 35) == 13Mb
            // 3422143*4/1024/1024
            
            
            // text pipeline:
            // 1. create a network (dont need to save it)
            // 2. hardcode and save hardcoded network
            // 3. make trainable sources out of hardcodednetwork
            // 4. train them, and dump the memory
            // 5. load hardcodednetwork
            // 6. restore its hardcodedcontext memory state with a dump
            // 7. make production sources with that hardcoded context
            
            
            // Estimated production network memory size (for float):
            //
            // (64, {128, 64, 128}, 64) == 2.5Mb
            // 631170*4/1024/1024
            //
            // (64, {128, 128, 128}, 64) == 4.2Mb (deeper connected)
            // 1099714*4/1024/1024
            // 594241*4/1024/1024 (actual)
            //
            // (64, {128, 256, 128}, 64) == 6Mb (7Mb deeper connected)
            // 1594434*4/1024/1024
            // 1856578*4/1024/1024
            // 998337*4/1024/1024 (3.8mb actual)
            //
            // (128, {128, 64, 128}, 128) == 3.4Mb
            // 902082*4/1024/1024
            //
            // (128, {128, 128, 128}, 128) == 4.6Mb
            // 1223170*4/1024/1024
            //
            // (128, {256, 256}, 128) == 8.4Mb
            // 2209410*4/1024/1024
            //
            // (128, {512}, 128) == 9.2Mb
            // 2406018*4/1024/1024
            //
            // (128, {128, 256, 128}, 128) == 7.6Mb
            // 2012802*4/1024/1024
            //
            // (256, {128, 64, 128}, 256) == 5.7Mb
            // 1493058*4/1024/1024
            //
            // (256, {128, 128, 128}, 256) == 7.3Mb
            // 1912450*4/1024/1024
            //
            // (256, {128, 256, 128}, 256) == 11Mb
            // 2898690*4/1024/1024
            //
            // 256, {256, 128, 256}, 256 == 13.7Mb
            // 3590018*4/1024/1024
            
            // Default network params (64 is the piano key range (88) minus lower and higher octaves (-24))
            int numInputs = 64;
            int numOutputs = 64;
            std::vector<int> hiddenLayers = { 128, 256, 128 };
            
            std::vector<int> sizes;
            int sizeParameterIndex = 2;
            
            while (args[sizeParameterIndex].getIntValue() > 0)
            {
                sizes.push_back(args[sizeParameterIndex].getIntValue());
                sizeParameterIndex++;
            }
            
            if (sizes.size() > 2)
            {
                numInputs = sizes.front();
                numOutputs = sizes.back();
                hiddenLayers.assign(sizes.begin() + 1, sizes.end() - 1);
            }
            
            TinyRNN::Network::Ptr network;
            TinyRNN::VMNetwork::Ptr vmNetwork;
            
            {
                ScopedTimer timer("Creating a network");
                network = TinyRNN::Network::Prefabs::longShortTermMemory("GoDeeper", numInputs, hiddenLayers, numOutputs);
            }
            
                {
                    ScopedTimer timer("Saving the topology");
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(network, TinyRNN::Keys::Core::Network)));
                    const File xmlFile(networkDirectory.getChildFile(topologyFileName));
                    xmlFile.replaceWithText(xmlString);
                }
            
            {
                ScopedTimer timer("Creating a vm-driven version");
                vmNetwork = network->toVM();
            }
            
                {
                    ScopedTimer timer("Saving the kernels");
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork, TinyRNN::Keys::VM::Network)));
                    const File xmlFile(networkDirectory.getChildFile(kernelsFileName));
                    xmlFile.replaceWithText(xmlString);
                }
            
                {
                    ScopedTimer timer("Saving the memory mapping");
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork->getContext(),
                                                                               TinyRNN::Keys::Hardcoded::TrainingContext)));
                    const File xmlFile(networkDirectory.getChildFile(mappingFileName + ".Full.xml"));
                    xmlFile.replaceWithText(xmlString);
                }
            
                {
                    ScopedTimer timer("Saving the lite memory mapping");
                    vmNetwork->getContext()->clearMappings();
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork->getContext(),
                                                                               TinyRNN::Keys::Hardcoded::TrainingContext)));
                    const File xmlFile(networkDirectory.getChildFile(mappingFileName));
                    xmlFile.replaceWithText(xmlString);
                    vmNetwork = nullptr;
                }
                
            {
                ScopedTimer timer("Creating a feed-only vm-driven version");
                vmNetwork = network->toFeedOnlyVM();
                network = nullptr;
            }
                
                {
                    ScopedTimer timer("Saving the kernels");
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork, TinyRNN::Keys::VM::Network)));
                    const File xmlFile(networkDirectory.getChildFile(kernelsFileName + "FeedOnly.xml"));
                    xmlFile.replaceWithText(xmlString);
                }
                
                {
                    ScopedTimer timer("Saving the memory mapping");
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork->getContext(),
                                                                               TinyRNN::Keys::Hardcoded::TrainingContext)));
                    const File xmlFile(networkDirectory.getChildFile(mappingFileName + ".FeedOnly.Full.xml"));
                    xmlFile.replaceWithText(xmlString);
                }
                
                {
                    ScopedTimer timer("Saving the lite memory mapping");
                    vmNetwork->getContext()->clearMappings();
                    PugiXMLSerializer serializer;
                    const std::string xmlString(std::move(serializer.serialize(vmNetwork->getContext(),
                                                                               TinyRNN::Keys::Hardcoded::TrainingContext)));
                    const File xmlFile(networkDirectory.getChildFile(mappingFileName + "FeedOnly.xml"));
                    xmlFile.replaceWithText(xmlString);
                    vmNetwork = nullptr;
                }
                
            std::cout << "All done." << std::endl;
            this->quit();
            return;
        }
        
        if (args[0].toLowerCase() == "midi" ||
            args[0].toLowerCase() == "text")
        {
            const File networkDirectory(File::getCurrentWorkingDirectory().getChildFile(args[1].unquoted()));
            const File kernelsFile(networkDirectory.getChildFile(kernelsFileName));
            const File mappingFile(networkDirectory.getChildFile(mappingFileName));
            const File latestMemDumpFile(networkDirectory.getChildFile(latestDumpFileName));
            
            File targetsDirectory(File::getCurrentWorkingDirectory().getChildFile("Targets"));
            File samplesDirectory(File::getCurrentWorkingDirectory().getChildFile("Samples"));
            
            if (args[2].toLowerCase() == "targets")
            {
                if (File::isAbsolutePath(args[3].unquoted()))
                {
                    targetsDirectory = File(args[3].unquoted());
                }
                else
                {
                    targetsDirectory = File::getCurrentWorkingDirectory().getChildFile(args[3].unquoted());
                }
            }
            
            if (! targetsDirectory.isDirectory())
            {
                std::cout << "Failed to find the targets at: " << targetsDirectory.getFullPathName() << std::endl;
                this->quit();
                return;
            }
            else
            {
                std::cout << "Using targets directory: " << targetsDirectory.getFullPathName() << std::endl;
            }
            
            if (! networkDirectory.isDirectory())
            {
                std::cout << "Failed to load network from: " << networkDirectory.getFullPathName() << std::endl;
                this->quit();
                return;
            }
           
            if (! kernelsFile.existsAsFile() ||
                ! mappingFile.existsAsFile())
            {
                std::cout << "Not found the network data at : " << networkDirectory.getFullPathName() << std::endl;
                this->quit();
                return;
            }
            
            TinyRNN::HardcodedTrainingContext::Ptr mappings(new TinyRNN::HardcodedTrainingContext());
            TinyRNN::VMNetwork::Ptr vmNetwork(new TinyRNN::VMNetwork(mappings));
            
            { // Here we do not need the mappings actually, only the rest stuff
                ScopedTimer timer("Loading the mapping");
                PugiXMLSerializer serializer;
                serializer.deserialize(mappings, mappingFile.loadFileAsString().toStdString());
            }
            
            if (latestMemDumpFile.existsAsFile())
            {
                ScopedTimer timer("Loading the latest memory dump");
                const std::string &memoryEncoded = latestMemDumpFile.loadFileAsString().toStdString();
                const std::vector<unsigned char> &memoryDecoded = TinyRNN::SerializationContext::decodeBase64(memoryEncoded);
                
                // What a hack
                //const size_t memoryVectorSize = (memoryDecoded.size() / sizeof(TinyRNN::Value));
                //mappings->getMemory().resize(memoryVectorSize);
                
                memcpy(mappings->getMemory().data(), memoryDecoded.data(), memoryDecoded.size());
            }
            
            {
                ScopedTimer timer("Loading the kernels");
                PugiXMLSerializer serializer;
                serializer.deserialize(vmNetwork, kernelsFile.loadFileAsString().toStdString());
            }
            
            if (args[0].toLowerCase() == "midi")
            {
                ScopedTimer timer("Training with BatchMidiProcessor");
                TrainingPipeline<BatchMidiProcessor> pipeline(vmNetwork, targetsDirectory, samplesDirectory, latestMemDumpFile);
                pipeline.start();
            }
            else if (args[0].toLowerCase() == "text")
            {
                ScopedTimer timer("Training with BatchTextProcessor");
                TrainingPipeline<BatchTextProcessor> pipeline(vmNetwork, targetsDirectory, samplesDirectory, latestMemDumpFile);
                pipeline.start();
            }
            
            this->quit();
            return;
        }
        
        Logger::writeToLog(commandLine);
        this->mainWindow = new MainWindow (this->getApplicationName());
    }
    
    virtual void shutdown() override
    {
        this->mainWindow = nullptr;
    }
    
    virtual void systemRequestedQuit() override
    {
        this->quit();
    }
    
    virtual void anotherInstanceStarted(const String& commandLine) override
    {
    }
    
    class MainWindow : public DocumentWindow
    {
    public:
        
        MainWindow(String name) : DocumentWindow(name,
                                                 Colours::lightgrey,
                                                 DocumentWindow::allButtons)
        {
            this->setUsingNativeTitleBar(true);
            this->setContentOwned(new MainComponent(), true);
            
            this->centreWithSize(getWidth(), getHeight());
            this->setVisible(true);
        }
        
        void closeButtonPressed() override
        {
            JUCEApplication::getInstance()->systemRequestedQuit();
        }
        
    private:
        
        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
    };
    
private:
    
    ScopedPointer<MainWindow> mainWindow;
};

START_JUCE_APPLICATION (GoDeeperApplication)
