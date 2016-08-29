using SideCar::Messages::Segment;
using SideCar::Messages::SegmentList;

// A simple tree structure.
// Knows that connections can only be formed between adjacent layers.
// Keeps all data in the first segment of the newest "flavor" layer.
// Merges all "leaves" into a single unsorted list.
class SegmentTree
{
public:
SegmentTree() : parent(this), flavor(false), elements(0) {}
SegmentTree(bool flavor) : parent(this), flavor(flavor), elements(0) {}
    ~SegmentTree()
    {
        if(elements)
            delete(elements);
    }

    bool isRoot() const { return parent==this; }
    SegmentTree * root()
    {
        if(isRoot())
            return this;

        return parent->root();
    }

    void seed(const Segment &s)
    {
        if(isRoot() && !elements)
        {
            elements=new SegmentList();
            elements->merge(s);
        }
        // otherwise this segment has already been added.
    }

    // merge two trees as necessary and add s into the mix.
    // it is assumed that "newer" is of the "current flavor".
    void add(SegmentTree &newer, const Segment &s)
    {
        SegmentTree *newTree=newer.root();
        SegmentTree *oldTree=root();

        // Merge all the elements together
        // increment the depth as necessary
        SegmentList *destination=0;
        {
            // three cases:
            // new and old elements -> merge them and delete one
            // only one set of elements-> use it
            // neither -> create a new tree
            if(newTree->elements && oldTree->elements)
            {
                destination=oldTree->elements;
                if(newTree!=oldTree)
                {
                    destination->merge(*newTree->elements);
                    delete(newTree->elements);
                }
            }
            else if(newTree->elements)
            {
                destination=newTree->elements;
            }
            else if(oldTree->elements)
            {
                destination=oldTree->elements;
            }
            else
            {
                destination=new SegmentList();
            }
        }
        destination->merge(s);

        // Put them in the appropriate spot
        if(flavor==newer.flavor)
        {
            newTree->elements=0;
            newTree->parent=oldTree;
            oldTree->elements=destination;
        }
        else
        {
            if(!newTree->elements)
                destination->setPRISpan(destination->PRISpan()+1);

            oldTree->elements=0;
            oldTree->parent=newTree;
            newTree->elements=destination;
        }
    }

    inline void setFlavor(bool f) { flavor=f; }
    inline bool isOld(bool newFlavor) const { return flavor!=newFlavor; }
    inline const SegmentList * data() const { return elements; }
    inline SegmentList * takeData()
    {
        SegmentList *tmp=elements;
        elements=0;
        return tmp;
    }

private:
    SegmentTree *parent; // root of the tree we are in
    bool flavor;
    SegmentList *elements; // only valid when root==this
};
