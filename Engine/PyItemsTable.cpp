/* ***** BEGIN LICENSE BLOCK *****
 * This file is part of Natron <http://www.natron.fr/>,
 * Copyright (C) 2016 INRIA and Alexandre Gauthier-Foichat
 *
 * Natron is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Natron is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
 * ***** END LICENSE BLOCK ***** */

// ***** BEGIN PYTHON BLOCK *****
// from <https://docs.python.org/3/c-api/intro.html#include-files>:
// "Since Python may define some pre-processor definitions which affect the standard headers on some systems, you must include Python.h before any standard headers are included."
#include <Python.h>
// ***** END PYTHON BLOCK *****

#include "PyItemsTable.h"

#if !defined(Q_MOC_RUN) && !defined(SBK_RUN)
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON
#endif


GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_OFF
#include "natronengine_python.h"
#include <shiboken.h> // produces many warnings
GCC_DIAG_UNUSED_LOCAL_TYPEDEFS_ON

#include "Engine/AppInstance.h"
#include "Engine/Bezier.h"
#include "Engine/Project.h"
#include "Engine/KnobItemsTable.h"
#include "Engine/PyNode.h"
#include "Engine/Node.h"
#include "Engine/PyRoto.h"
#include "Engine/RotoStrokeItem.h"
#include "Engine/TrackMarker.h"
#include "Engine/PyTracker.h"

NATRON_NAMESPACE_ENTER;
NATRON_PYTHON_NAMESPACE_ENTER;

ItemBase::ItemBase(const KnobTableItemPtr& item)
: _item(item)
{
    assert(item);
}

ItemBase::~ItemBase()
{

}


template <typename VIEWSPECTYPE>
static bool getViewSpecFromViewNameInternal(const ItemBase* item, bool allowAll, const QString& viewName, VIEWSPECTYPE* view) {

    if (allowAll && viewName == QLatin1String(kPyParamViewSetSpecAll)) {
        *view = VIEWSPECTYPE(ViewSetSpec::all());
        return true;
    } else if (viewName == QLatin1String(kPyParamViewIdxMain) ) {
        *view = VIEWSPECTYPE(0);
        return true;
    }
    KnobTableItemPtr internalItem = item->getInternalItem();
    if (!internalItem) {
        return false;
    }

    AppInstancePtr app = internalItem->getApp();
    if (!app) {
        return false;
    }
    const std::vector<std::string>& projectViews = app->getProject()->getProjectViewNames();
    int i = 0;
    bool foundView = false;
    ViewIdx foundViewIdx;
    std::string stdViewName = viewName.toStdString();
    foundView = Project::getViewIndex(projectViews, stdViewName, &foundViewIdx);

    for (std::vector<std::string>::const_iterator it2 = projectViews.begin(); it2 != projectViews.end(); ++it2, ++i) {
        if (boost::iequals(*it2, stdViewName)) {

            foundViewIdx = ViewIdx(i);
            foundView = true;
            break;
        }
    }

    if (!foundView) {
        return false;
    }

    // Now check that the view exist in the knob
    std::list<ViewIdx> splitViews = internalItem->getViewsList();
    for (std::list<ViewIdx>::const_iterator it = splitViews.begin(); it != splitViews.end(); ++it) {
        if (*it == foundViewIdx) {
            *view = VIEWSPECTYPE(foundViewIdx);
            return true;
        }
    }
    *view = VIEWSPECTYPE(0);
    return true;
}

bool
ItemBase::getViewGetSpecFromViewName(const QString& viewName, ViewGetSpec* view) const
{
    return getViewSpecFromViewNameInternal(this, false, viewName, view);
}

bool
ItemBase::getViewSetSpecFromViewName(const QString& viewName, ViewSetSpec* view) const
{
    return getViewSpecFromViewNameInternal(this, true, viewName, view);
}

void
ItemBase::setLabel(const QString & name)
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return;
    }
    item->setLabel(name.toStdString(), eTableChangeReasonInternal);
}

QString
ItemBase::getLabel() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(item->getLabel().c_str());
}

void
ItemBase::setIconFilePath(const QString& icon)
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return;
    }
    item->setIconLabelFilePath(icon.toStdString(), eTableChangeReasonInternal);
}

QString
ItemBase::getIconFilePath() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(item->getIconLabelFilePath().c_str());

}

QString
ItemBase::getScriptName() const{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(item->getScriptName_mt_safe().c_str());

}

ItemBase*
ItemBase::getParent() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return 0;
    }
    KnobTableItemPtr parent = item->getParent();
    if (!parent) {
        return 0;
    }
    return ItemsTable::createPyItemWrapper(parent);
}

int
ItemBase::getIndexInParent() const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return -1;
    }
    return item->getIndexInParent();
}

std::list<ItemBase*>
ItemBase::getChildren() const
{
    std::list<ItemBase*> ret;
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return ret;
    }
    std::vector<KnobTableItemPtr> children = item->getChildren();
    for (std::size_t i = 0; i < children.size(); ++i) {
        ItemBase* item = ItemsTable::createPyItemWrapper(children[i]);
        if (item) {
            ret.push_back(item);
        }
    }
    return ret;
}

std::list<Param*>
ItemBase::getParams() const
{
    std::list<Param*> ret;
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return ret;
    }

    const KnobsVec& knobs = item->getKnobs();

    for (KnobsVec::const_iterator it = knobs.begin(); it != knobs.end(); ++it) {
        Param* p = Effect::createParamWrapperForKnob(*it);
        if (p) {
            ret.push_back(p);
        }
    }

    return ret;

}

Param*
ItemBase::getParam(const QString& name) const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return 0;
    }

    KnobIPtr knob = item->getKnobByName( name.toStdString() );
    if (knob) {
        return Effect::createParamWrapperForKnob(knob);
    } else {
        return NULL;
    }
}

void
ItemBase::getUserKeyframes(std::list<double>* keys, const QString& view) const
{
    KnobTableItemPtr item = getInternalItem();
    if (!item || !keys) {
        PythonSetNullError();
        return;
    }

    ViewGetSpec viewSpec;
    if (!getViewGetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    std::set<double> times;
    item->getMasterKeyFrameTimes(viewSpec, &times);
    for (std::set<double>::const_iterator it = times.begin(); it != times.end(); ++it) {
        keys->push_back(*it);
    }
}

void
ItemBase::setUserKeyframe(double frame, const QString& view)
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return;
    }

    if (!item->getCanAnimateUserKeyframes()) {
        PyErr_SetString(PyExc_ValueError, tr("Cannot set user keyframe on an item that cannot animate").toStdString().c_str());
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->setKeyFrame(frame, viewSpec, 0);
}

void
ItemBase::deleteUserKeyframe(double frame, const QString& view)
{
    KnobTableItemPtr item = getInternalItem();
    if (!item) {
        PythonSetNullError();
        return;
    }

    if (!item->getCanAnimateUserKeyframes()) {
        PyErr_SetString(PyExc_ValueError, tr("Cannot remove user keyframe on an item that cannot animate").toStdString().c_str());
        return;
    }

    ViewSetSpec viewSpec;
    if (!getViewSetSpecFromViewName(view, &viewSpec)) {
        PythonSetInvalidViewName(view);
        return;
    }
    item->deleteValueAtTime(frame, viewSpec, DimSpec::all());

}


ItemsTable::ItemsTable(const KnobItemsTablePtr& table)
: _table(table)
{
    
}

ItemsTable::~ItemsTable()
{

}


ItemBase*
ItemsTable::getItemByFullyQualifiedScriptName(const QString& name) const
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }
    KnobTableItemPtr item = model->getItemByFullyQualifiedScriptName(name.toStdString());
    if (!item) {
        return 0;
    }
    return createPyItemWrapper(item);
}

std::list<ItemBase*>
ItemsTable::getTopLevelItems() const
{
    std::list<ItemBase*> ret;
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return ret;
    }
    std::vector<KnobTableItemPtr> children = model->getTopLevelItems();
    for (std::size_t i = 0; i < children.size(); ++i) {
        ItemBase* item = createPyItemWrapper(children[i]);
        if (item) {
            ret.push_back(item);
        }
    }
    return ret;
}

std::list<ItemBase*>
ItemsTable::getSelectedItems() const
{
    std::list<ItemBase*> ret;
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return ret;
    }
    std::list<KnobTableItemPtr> selection = model->getSelectedItems();
    for (std::list<KnobTableItemPtr>::const_iterator it = selection.begin(); it!=selection.end(); ++it) {
        ItemBase* item = createPyItemWrapper(*it);
        if (item) {
            ret.push_back(item);
        }
    }
    return ret;
}

ItemBase*
ItemsTable::createPyItemWrapper(const KnobTableItemPtr& item)
{

    if (!item) {
        return 0;
    }

    KnobItemsTablePtr model = item->getModel();
    if (!model) {
        PythonSetNullError();
        return 0;
    }

    NodePtr node = model->getNode();
    if (!node) {
        PythonSetNullError();
        return 0;
    }

    // First, try to re-use an existing ItemsTable object that was created for this node.
    // If not found, create one.
    std::stringstream ss;
    ss << kPythonTmpCheckerVariable << " = ";
    ss << node->getApp()->getAppIDString() << "." << node->getFullyQualifiedName() << "." << model->getPythonPrefix();
    ss << "." << item->getFullyQualifiedName();
    std::string script = ss.str();
    bool ok = NATRON_PYTHON_NAMESPACE::interpretPythonScript(script, 0, 0);
    // Clear errors if our call to interpretPythonScript failed, we don't want the
    // calling function to fail aswell.
    PyErr_Clear();
    if (ok) {
        PyObject* pyItem = 0;
        PyObject* mainModule = NATRON_PYTHON_NAMESPACE::getMainModule();
        if ( PyObject_HasAttrString(mainModule, kPythonTmpCheckerVariable) ) {
            pyItem = PyObject_GetAttrString(mainModule, kPythonTmpCheckerVariable);
        }
        ItemBase* cppItem = 0;
        if (pyItem && Shiboken::Object::isValid(pyItem)) {
            cppItem = (ItemBase*)Shiboken::Conversions::cppPointer(SbkNatronEngineTypes[SBK_ITEMBASE_IDX], (SbkObject*)pyItem);
        }
        NATRON_PYTHON_NAMESPACE::interpretPythonScript("del " kPythonTmpCheckerVariable, 0, 0);

        if (cppItem) {
            return cppItem;
        }
    }


    BezierPtr isBezier = toBezier(item);
    if (isBezier) {
        return new BezierCurve(isBezier);
    }
    RotoStrokeItemPtr isStroke = toRotoStrokeItem(item);
    if (isStroke) {
        return new StrokeItem(isStroke);
    }

    TrackMarkerPtr isTrack = toTrackMarker(item);
    if (isTrack) {
        return new Track(isTrack);
    }
    return new ItemBase(item);
}

void
ItemsTable::insertItem(int index, const ItemBase* item, const ItemBase* parent)
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return ;
    }
    KnobTableItemPtr iptr = item->getInternalItem();
    if (!iptr) {
        PythonSetNullError();
        return;
    }
    KnobTableItemPtr parentItm;
    if (parent) {
        parentItm = parent->getInternalItem();
    }
    if (parentItm && model->getType() == KnobItemsTable::eKnobItemsTableTypeTable) {
        PyErr_SetString(PyExc_ValueError, tr("This item cannot have a parent").toStdString().c_str());
        return;
    }

    model->insertItem(index, iptr, parentItm, eTableChangeReasonInternal);
}

void
ItemsTable::removeItem(const ItemBase* item)
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return ;
    }
    KnobTableItemPtr iptr = item->getInternalItem();
    if (!iptr) {
        PythonSetNullError();
        return;
    }
    model->removeItem(iptr, eTableChangeReasonInternal);
}

QString
ItemsTable::getAttributeName() const
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return QString();
    }
    return QString::fromUtf8(model->getPythonPrefix().c_str());
}

bool
ItemsTable::isModelParentingEnabled() const
{
    KnobItemsTablePtr model = getInternalModel();
    if (!model) {
        PythonSetNullError();
        return false;
    }
    return model->getType() == KnobItemsTable::eKnobItemsTableTypeTree;
}

NATRON_PYTHON_NAMESPACE_EXIT;
NATRON_NAMESPACE_EXIT;