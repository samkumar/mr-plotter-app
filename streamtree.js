function filterTrivialKeywords(keywords) {
    for (var j = keywords.length - 1; j !== -1; j--) {
        keywords[j] = keywords[j].trim();
        if (keywords[j] === "") {
            keywords.splice(j, 1);
        }
    }
}

function setCollectionsModel(collectionNames) {
    for (var i = 0; i !== collectionNames.length; i++) {
        var collection = {
            "name": collectionNames[i],
            "opened": false,
            "streamsLoaded": false,
            "streamsRequestPending": false,
            "streamList": [],
            "collectionMatchesQuery": true
        };
        collectionsModel.append(collection);
    }
}

function collectionMatchesKeywords(collection, keywords) {
    var name = collection.name;
    for (var i = 0; i !== keywords.length; i++) {
        var keyword = keywords[i];
        if (!name.includes(keyword)) {
            return false;
        }
    }
    return true;
}

function applyCollectionSearch(keywords) {
    for (var i = 0; i !== collectionsModel.count; i++) {
        var matches = collectionMatchesKeywords(collectionsModel.get(i), keywords);
        collectionsModel.setProperty(i, "collectionMatchesQuery", matches);
    }
}

function mapMatchesKeyword(map, keyword) {
    for (var key in map) {
        var value = map[key];
        if (key.includes(keyword) || value.includes(keyword)) {
            return true;
        }
    }
    return false;
}

function streamMatchesKeyword(stream, keyword) {
    var details = stream.streamObject.details;
    return details.UUID.includes(keyword) ||
            mapMatchesKeyword(details.tags, keyword) ||
            mapMatchesKeyword(details.annotations, keyword);
}

function streamMatchesKeywords(stream, keywords) {
    for (var i = 0; i !== keywords.length; i++) {
        var keyword = keywords[i];
        if (!streamMatchesKeyword(stream, keyword)) {
            return false;
        }
    }
    return true;
}

function applyStreamSearch(keywords) {
    for (var i = 0; i !== collectionsModel.count; i++) {
        var collection = collectionsModel.get(i);
        var streamList = collection.streamList;
        for (var j = 0; j !== streamList.count; j++) {
            var matches = streamMatchesKeywords(streamList.get(j), keywords);
            streamList.setProperty(j, "streamMatchesQuery", matches);
        }
    }
}
