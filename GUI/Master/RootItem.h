#ifndef SIDECAR_GUI_ROOTITEM_H // -*- C++ -*-
#define SIDECAR_GUI_ROOTITEM_H

#include "CollectionItem.h"

namespace SideCar {
namespace GUI {
namespace Master {

class ConfigurationItem;

/** The root item of a QTreeView view. A RootItem object has no parent, and all children are of class
    ConfigurationItem.
*/
class RootItem : public CollectionItem {
    Q_OBJECT
    using Super = CollectionItem;

public:
    /** Constructor.
     */
    RootItem() : Super() {}

    /** Locate a child item by its name.

        \param name the name to look forward

        \return found child or NULL if not found
    */
    ConfigurationItem* find(const QString& name) const;

    ConfigurationItem* getChild(int index) const;

private:
    void updateChildren() {}
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
