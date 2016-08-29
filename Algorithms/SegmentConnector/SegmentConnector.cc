#include "Messages/RadarConfig.h"

#include "SegmentConnector.h"

using namespace SideCar;
using namespace SideCar::Messages;
using namespace SideCar::Algorithms;

static const char* kAlgorithmName = "SegmentConnector";

SegmentConnector::SegmentConnector(Controller& controller, Logger::Log& log)
    : Algorithm(controller, log), oldPRI(),
      oldTrees(new std::list<SegmentTree *>())
{
    ;
}

bool
SegmentConnector::startup()
{
    registerProcessor<SegmentConnector,SegmentMessage>(
	&SegmentConnector::process);
    return Algorithm::startup();
}

bool
SegmentConnector::reset()
{
    oldPRI.reset();
    oldTrees->clear();
    return true;
}

bool
SegmentConnector::process(Messages::SegmentMessage::Ref newPRI)
{
    // Detect changes in runtime settings, resetting everything when they occur.
    //
    if (oldPRI && (rangeMin_ != newPRI->getRangeMin() ||
                   rangeFactor_ != newPRI->getRangeFactor())) {
	reset();
    }

    if (! oldPRI) {
	rangeMin_ = newPRI->getRangeMin();
	rangeFactor_ = newPRI->getRangeFactor();
	oldPRI = boost::shared_ptr<SegmentMessage>(
	    new SegmentMessage(kAlgorithmName, newPRI, rangeMin_,
                               rangeFactor_));
    }

    // pick a different flavor each month, but always chocolate or vanilla
    bool flavor=false;
    if(!oldTrees->empty())
    {
        flavor=oldTrees->front()->isOld(true); // toggles the flavor
    }

    // Initialize the new list of trees
    std::list<SegmentTree *> *newTrees=new std::list<SegmentTree *>(newPRI->data()->data().size());
    { // initialize the new element flavors
        std::list<SegmentTree *>::iterator i;
        std::list<SegmentTree *>::const_iterator stop=newTrees->end();
        for(i=newTrees->begin(); i!=stop; i++)
        {
            *i=new SegmentTree(flavor);
        }
    }

    // Indices into both segment lists
    std::list<Segment>::const_iterator oldSegment=oldPRI->data()->data().begin();
    std::list<Segment>::const_iterator newSegment=newPRI->data()->data().begin();
    std::list<Segment>::const_iterator stopOld=oldPRI->data()->data().end();
    std::list<Segment>::const_iterator stopNew=newPRI->data()->data().end();

    // Indices into both tree lists
    std::list<SegmentTree *>::iterator oldTree=oldTrees->begin(); // previous connections
    std::list<SegmentTree *>::iterator newTree=newTrees->begin(); // previous connections

    // Connect adjacent segments -- assumes that segments within a layer are maximally connected
    // i.e. gaps separate all segments within a layer
    for(; oldSegment!=stopOld; oldSegment++, oldTree++)
    {
        for(; newSegment!=stopNew; newSegment++, newTree++)
        {
            if(newSegment->stop < oldSegment->start - 1) // -1 gives full 8-point connectivity
            {
                // new segment ends before old segment begins -- start a new extraction as needed
                (*newTree)->seed(*newSegment);
                continue;
            }

            if(newSegment->start > oldSegment->stop + 1) // +1 gives full 8-point connectivity
            {
                // new segment begins after old segment ends -- check the next old segment
                break;
            }

            // the segments overlap -- merge
            (*oldTree)->add(**newTree, *newSegment);

            if(newSegment->stop > oldSegment->stop + 1)
            {
                // new ends after old segment ends -- may touch the next segment
                break;
            }
        }

        if(newSegment==stopNew)
        {
            // no more segments to process
            break;
        }
    }
    // clean up any untouched new segments
    for(; newSegment!=stopNew; newSegment++, newTree++)
    {
        (*newTree)->seed(*newSegment);
    }

    // Identify finished (not updated this PRI) extractions
    std::list<SegmentTree *>::const_iterator stopTree=oldTrees->end();
    for(oldTree=oldTrees->begin(); oldTree!=stopTree; oldTree++)
    {
        if((*oldTree)->isRoot())
        {
            // extraction is complete
            if(! send(SegmentMessage::Ref(
                          new SegmentMessage(kAlgorithmName,
                                             newPRI,
                                             (*oldTree)->takeData(),
                                             rangeMin_, rangeFactor_))))
                return false;
        }
    }

    // Identify rings
    // anything over half a scan is "ring-like"
    const size_t halfScan = (RadarConfig::GetShaftEncodingMax() + 1) / 2;
    stopTree=newTrees->end();
    for(newTree=newTrees->begin(); newTree!=stopTree; /* increment in the loop */)
    {
        if((*newTree)->isRoot() && ((*newTree)->data()->PRISpan() > halfScan))
        {
            // remove the corresponding segments
            newPRI->data()->pop(*(*newTree)->data());
            // extraction is complete
            if(! send(SegmentMessage::Ref(
                          new SegmentMessage(kAlgorithmName,
                                             newPRI,
                                             (*newTree)->takeData(),
                                             rangeMin_, rangeFactor_))))
                return false;

            // remove all traces of the extraction from newTrees
            // get the name of the tree to remove
            SegmentTree *dead=(*newTree)->root();
            // perform the cut
            std::list<SegmentTree *>::iterator cull;
            for(cull=newTrees->begin(); cull!=stopTree; cull++)
            {
                if((*cull)->root() == dead)
                {
                    if(cull==newTree)
                        newTree++;
                    cull=newTrees->erase(cull);
                }
            }
        }
        else
        {
            newTree++;
        }
    }

    // Prepare for the next PRI
    oldPRI=newPRI;
    stopTree=oldTrees->end();
    for(oldTree=oldTrees->begin(); oldTree!=stopTree; oldTree++)
    {
        delete(*oldTree);
    }
    delete(oldTrees);
    oldTrees=newTrees;
    return true;
}

// DLL support
//
extern "C" ACE_Svc_Export Algorithm*
SegmentConnectorMake(Controller& controller, Logger::Log& log)
{
    return new SegmentConnector(controller, log);
}
