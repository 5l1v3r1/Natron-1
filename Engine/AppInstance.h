//  Natron
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/*
 * Created by Alexandre GAUTHIER-FOICHAT on 6/1/2012.
 * contact: immarespond at gmail dot com
 *
 */

#ifndef APPINSTANCE_H
#define APPINSTANCE_H


#include <vector>
#include <list>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <QStringList>

#include "Global/GlobalDefines.h"

namespace boost {
namespace archive {
class xml_iarchive;
class xml_oarchive;
}
}

class NodeSerialization;
class TimeLine;
struct AppInstancePrivate;
class KnobSerialization;
class ProcessHandler;
namespace Natron {
class Node;
class Project;
class EffectInstance;
class OutputEffectInstance;
}

struct CreateNodeArgs
{
    ///The pluginID corresponds to something generated by
    ///OfxEffectInstance::generateImageEffectClassName
    ///or given from the virtual function getPluginID() of the builtin plugins (such as Backdrop,Viewer,etc...)
    QString pluginID;
    int majorV,minorV;
    std::string multiInstanceParentName;
    int childIndex;
    bool autoConnect;
    double xPosHint,yPosHint;
    bool pushUndoRedoCommand;
    bool addToProject;
    QString fixedName;
    
    typedef std::list< boost::shared_ptr<KnobSerialization> > DefaultValuesList;
    DefaultValuesList paramValues; //< values of parameters to set before creating the plug-in
    
    ///Constructor used to create a new node
    explicit CreateNodeArgs(const QString & pluginID, //< the pluginID (as they appear in the "Tab" menu in the nodegraph)
                            const std::string & multiInstanceParentName, //< no parent by default DEFAULT = empty
                            int majorVersion , //< use greatest version found DEFAULT = - 1
                            int minorVersion , //< use greatest version found DEFAULT = -1
                            int childIndex ,//< if this is a child of a multi-instance, this is the index DEFAULT = -1
                            bool autoConnect , //< should we attempt to autoconnect ? DEFAULT = true
                            double xPosHint, //< xPosition in the nodegraph DEFAULT = INT_MIN
                            double yPosHint, //< yPosition in the nodegraph DEFAULT = INT_MIN
                            bool pushUndoRedoCommand , //< should we push a new undo/redo command on the GUI? DEFAULT = true
                            bool addToProject, //< should we add the node to the project ? DEFAULT = true
                            const QString & fixedName,  //< if non empty, this will be the name of the node DEFAULT = empty
                            const DefaultValuesList& paramValues) //< parameters to set before creating the plugin
        : pluginID(pluginID)
          , majorV(majorVersion)
          , minorV(minorVersion)
          , multiInstanceParentName(multiInstanceParentName)
          , childIndex(childIndex)
          , autoConnect(autoConnect)
          , xPosHint(xPosHint)
          , yPosHint(yPosHint)
          , pushUndoRedoCommand(pushUndoRedoCommand)
          , addToProject(addToProject)
          , fixedName(fixedName)
          , paramValues(paramValues)
    {
    }
    
};


struct LoadNodeArgs
{
    QString pluginID;
    int majorV,minorV;
    bool dontLoadName;
    std::string multiInstanceParentName;
    const NodeSerialization* serialization;

    ///Constructor used to load a node from the project serialization
    LoadNodeArgs(const QString & pluginID,
                 const std::string & multiInstanceParentName,
                 int majorVersion,
                 int minorVersion,
                 const NodeSerialization* serialization,
                 bool dontLoadName)
        : pluginID(pluginID)
          , majorV(majorVersion)
          , minorV(minorVersion)
          , dontLoadName(dontLoadName) //< used when copy/pasting nodes to avoid duplicates in names
          , multiInstanceParentName(multiInstanceParentName)
          , serialization(serialization)
    {
    }
};

class AppInstance
    : public QObject, public boost::noncopyable
{
    Q_OBJECT

public:


    AppInstance(int appID);

    virtual ~AppInstance();

    virtual void aboutToQuit()
    {
    }

    struct RenderRequest {
        QString writerName;
        int firstFrame,lastFrame;
    };
    
    struct RenderWork {
        Natron::OutputEffectInstance* writer;
        int firstFrame,lastFrame;
    };
    
    virtual void load(const QString & projectName = QString(), const std::list<RenderRequest> &writersWork = std::list<RenderRequest>() );

    int getAppID() const;

    /** @brief Create a new node  in the node graph.
     * The name passed in parameter must match a valid node name,
     * otherwise an exception is thrown. You should encapsulate the call
     * by a try-catch block.
     * If the majorVersion is not -1 then this function will attempt to find a plugin with the matching
     * majorVersion, or otherwise it will throw an exception.
     * If the minorVersion is not -1 then this function will attempt to load a plugin with the greatest minorVersion
     * greater or equal to this minorVersion.
     * By default this function also create the node's graphical user interface and attempts to automatically
     * connect this node to other nodes selected.
     * If requestedByLoad is true then it will never attempt to do this auto-connection.
     * If openImageFileDialog is true then if the node has a file knob indicating an image file it will automatically
     * prompt the user with a file dialog.
     *
     * @param createGui If false, the node GUI (in the nodegraph and in the properties panel) will not be created.
     * The node animation curves will still be available in the curve editor though.
     *
     * You can use this function to create backdrops also which are purely GUI stuff. In this case the pointer returned will
     * be NULL.
     **/
    boost::shared_ptr<Natron::Node> createNode(const CreateNodeArgs & args);

    ///Same as createNode but used when loading a project
    boost::shared_ptr<Natron::Node> loadNode(const LoadNodeArgs & args);

    void getActiveNodes(std::vector<boost::shared_ptr<Natron::Node> > *activeNodes) const;

    /**
     * @brief Returns a pointer to a node whose name is the same as the name given in parameter.
     * If no such node could be found, NULL is returned.
     **/
    boost::shared_ptr<Natron::Node> getNodeByName(const std::string & name) const;
    boost::shared_ptr<Natron::Project> getProject() const;
    boost::shared_ptr<TimeLine> getTimeLine() const;

    /*true if the user is NOT scrubbing the timeline*/
    virtual bool shouldRefreshPreview() const
    {
        return false;
    }

    virtual void connectViewersToViewerCache()
    {
    }

    virtual void disconnectViewersFromViewerCache()
    {
    }

    virtual void errorDialog(const std::string & title,const std::string & message) const;
    virtual void warningDialog(const std::string & title,const std::string & message) const;
    virtual void informationDialog(const std::string & title,const std::string & message) const;
    virtual Natron::StandardButton questionDialog(const std::string & title,const std::string & message,Natron::StandardButtons buttons =
                                                      Natron::StandardButtons(Natron::Yes | Natron::No),
                                                  Natron::StandardButton defaultButton = Natron::NoButton) const WARN_UNUSED_RETURN;
    virtual void loadProjectGui(boost::archive::xml_iarchive & /*archive*/) const
    {
    }

    virtual void saveProjectGui(boost::archive::xml_oarchive & /*archive*/)
    {
    }

    virtual void setupViewersForViews(int /*viewsCount*/)
    {
    }

    virtual void notifyRenderProcessHandlerStarted(const QString & /*sequenceName*/,
                                                   int /*firstFrame*/,
                                                   int /*lastFrame*/,
                                                   const boost::shared_ptr<ProcessHandler> & /*process*/)
    {
    }

    virtual bool isShowingDialog() const
    {
        return false;
    }
    
    virtual bool isGuiFrozen() const { return false; }

    virtual void startProgress(Natron::EffectInstance* /*effect*/,
                               const std::string & /*message*/)
    {
    }

    virtual void endProgress(Natron::EffectInstance* /*effect*/)
    {
    }

    virtual bool progressUpdate(Natron::EffectInstance* /*effect*/,
                                double /*t*/)
    {
        return true;
    }

    /**
     * @brief Checks for a new version of Natron
     **/
    void checkForNewVersion() const;
    virtual void onMaxPanelsOpenedChanged(int /*maxPanels*/)
    {
    }

    Natron::ViewerColorSpace getDefaultColorSpaceForBitDepth(Natron::ImageBitDepth bitdepth) const;

    int getMainView() const;

    /**
     * @brief Clears any shared ptr to NodeGuis left
     **/
    virtual void clearNodeGuiMapping()
    {
    }

    virtual void aboutToAutoSave()
    {
    }

    virtual void autoSaveFinished()
    {
    }

    virtual std::string openImageFileDialog() { return std::string(); }
    virtual std::string saveImageFileDialog() { return std::string(); }

    
    void onOCIOConfigPathChanged(const std::string& path);
    
  
    
    void startWritersRendering(const std::list<RenderRequest>& writers);
    void startWritersRendering(const std::list<RenderWork>& writers);

    virtual void startRenderingFullSequence(const RenderWork& writerWork,bool renderInSeparateProcess,const QString& savePath);


public slots:

    void quit();

    /* The following methods are forwarded to the model */
    void checkViewersConnection();

    void redrawAllViewers();

    void triggerAutoSave();

    void clearOpenFXPluginsCaches();

    void clearAllLastRenderedImages();

    void clearViewersLastRenderedTexture();

    void newVersionCheckDownloaded();

    void newVersionCheckError();

signals:

    void pluginsPopulated();

protected:

    virtual void createNodeGui(boost::shared_ptr<Natron::Node> /*node*/,
                               const std::string & /*multiInstanceParentName*/,
                               bool /*loadRequest*/,
                               bool /*autoConnect*/,
                               double /*xPosHint*/,
                               double /*yPosHint*/,
                               bool /*pushUndoRedoCommand*/)
    {
    }
    


private:

    /**
     * @brief Override to create a backdrop node. This is purely GUI.
     **/
    virtual void createBackDrop()
    {
    }

    boost::shared_ptr<Natron::Node> createNodeInternal(const QString & pluginID,const std::string & multiInstanceParentName,
                                                       int majorVersion,int minorVersion,
                                                       bool requestedByLoad,
                                                       const NodeSerialization & serialization,bool dontLoadName,
                                                       int childIndex,bool autoConnect,double xPosHint,double yPosHint,
                                                       bool pushUndoRedoCommand,bool addToProject,const QString& fixedName,
                                                       const CreateNodeArgs::DefaultValuesList& paramValues);
    boost::scoped_ptr<AppInstancePrivate> _imp;
};


#endif // APPINSTANCE_H
