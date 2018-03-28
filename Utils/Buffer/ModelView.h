#ifndef BUFFER_MODELVIEW_H
#define BUFFER_MODELVIEW_H

// This file outlines a model/view framework between Buffer and its users

#include <list>

class BufferView;

/**
   subclassed by Buffer.
*/
class BufferModel {
    friend class BufferView;

protected:
    using ViewList = std::list<const BufferView*>;
    ViewList views; // Keep track of all objects using this buffer

private:
    /**
     * Register an object for persistent use with this Buffer.
     */
    ViewList::iterator connect(const BufferView* bv)
    {
        views.push_back(bv);
        return views.rbegin().base();
    }
    /**
     * Release a registered object from use with this Buffer.
     */
    void disconnect(ViewList::iterator& connection) { views.erase(connection); }
};

/**
   subclassed by Row, Col, Matrix, etc...
*/
class BufferView {
    friend class BufferModel;

protected:
    /**
       Signals that the Model is changing and the View needs to flush its work.
    */
    virtual void pause() = 0;
    /**
       Signals that the Model has finished changing and that the View can rebind.
    */
    virtual void resume() = 0;

    BufferView(BufferModel& m) : model(m), connection(model.connect(this)) {}
    virtual ~BufferView() { model.disconnect(connection); }

private:
    BufferModel& model;
    BufferModel::ViewList::iterator connection;
};

/** \file
 */

#endif
