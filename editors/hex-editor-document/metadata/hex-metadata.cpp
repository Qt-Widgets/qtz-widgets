#include "hex-metadata.hpp"
#include <QMutableHashIterator>
#include <functional>

typedef QMutableHashIterator<integer_t, MetadataList> MetadataIterator;

#define RangeContains(offset, it) (offset >= it.key() && offset <= it.value())

static void removeMetadata(MetadataMap& metadata, std::function<bool(HexMetadataItem*)> cb) {
    MetadataIterator it(metadata);

    while(it.hasNext()) {
        it.next();
        sinteger_t i = 0;
        MetadataList& values = metadata[it.key()];

        while(i < values.length()) {
            if(cb(values[i])) {
                values.removeAt(i);
            } else {
                i++;
            }
        }
    }
}

HexMetadata::HexMetadata(QObject* parent) : QObject(parent), _bulkmetadata(false) {

}

void HexMetadata::insert(HexMetadataItem* metaitem) {
    if(!this->_metadata.contains(metaitem->startOffset())) {
        this->_metadata[metaitem->startOffset()] = MetadataList();
        this->_ranges[metaitem->startOffset()] = 0;
    }

    this->_metadata[metaitem->startOffset()].append(metaitem);
    this->_ranges[metaitem->startOffset()] = qMax(this->_ranges[metaitem->startOffset()], metaitem->endOffset());

    if(!this->_bulkmetadata) {
        emit metadataChanged();
    }
}

void HexMetadata::beginMetadata() {
    this->_bulkmetadata = true;
}

void HexMetadata::endMetadata() {
    this->_bulkmetadata = false;
    emit metadataChanged();
}

void HexMetadata::clearHighlighting() {
    if(this->_metadata.isEmpty()) {
        return;
    }

    removeMetadata(this->_metadata, [](HexMetadataItem * metaitem) -> bool {
        if(metaitem->hasComment()) {
            metaitem->clearColors();
            return false;
        }

        return true;
    });

    if(!this->_bulkmetadata) {
        emit metadataChanged();
    }
}

void HexMetadata::clearComments() {
    if(this->_metadata.isEmpty()) {
        return;
    }

    removeMetadata(this->_metadata, [this](HexMetadataItem * metaitem) -> bool {
        bool doremove = false;

        if(metaitem->hasForeColor() || metaitem->hasBackColor())
            metaitem->clearComment();
        else {
            doremove = true;
        }

        return doremove;
    });

    if(!this->_bulkmetadata) {
        emit metadataChanged();
    }
}

MetadataList HexMetadata::fromOffset(integer_t offset) const {
    if(this->_ranges.isEmpty()) {
        return MetadataList();
    }

    MetadataList metadata;

    for(RangeMap::const_iterator it = this->_ranges.begin(); it != this->_ranges.end(); it++) {
        if(it.key() > offset) {
            break;
        }

        if(!RangeContains(offset, it)) {
            continue;
        }

        metadata += this->_metadata[it.key()];
    }

    return metadata;
}

QString HexMetadata::commentString(integer_t offset) const {
    const HexMetadataItem* metadata = this->comment(offset);

    if(metadata) {
        return metadata->comment();
    }

    return QString();
}

const HexMetadataItem* HexMetadata::comment(integer_t offset) const {
    MetadataList metalist = this->fromOffset(offset);

    for(MetadataList::const_reverse_iterator it = metalist.crbegin(); it != metalist.crend(); it++) {
        if((*it)->hasComment()) {
            return *it;
        }
    }

    return NULL;
}
