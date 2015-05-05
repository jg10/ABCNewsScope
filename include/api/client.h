#ifndef API_CLIENT_H_
#define API_CLIENT_H_

#include <api/config.h>

#include <atomic>
#include <deque>
#include <map>
#include <string>
#include <core/net/http/request.h>
#include <core/net/uri.h>

#include <QXmlStreamReader>

namespace api {

/**
 * Provide a nice way to access the HTTP API.
 *
 * We don't want our scope's code to be mixed together with HTTP and JSON handling.
 */
class Client {
public:

    /**
     * News Item Information
     */
    struct NewsItem {
        std::string title;
        std::string link;
        std::string description;
        std::string image;
    };


    /**
     * A list of news items
     */
    typedef std::deque<NewsItem> NewsItemList;

    /**
     * News Item Results
     */
    struct NewsItemRes {
        NewsItemList newsItems;
    };


    Client(Config::Ptr config);

    virtual ~Client() = default;

    /**
     * Get the current news items
     */
    virtual NewsItemRes newsItems(const std::string &query, const std::string &category);


    /**
     * Cancel any pending queries (this method can be called from a different thread)
     */
    virtual void cancel();

    virtual Config::Ptr config();

protected:
    void get(const core::net::Uri::Path &path,
             const core::net::Uri::QueryParameters &parameters,
             QXmlStreamReader &reader);
    /**
     * Progress callback that allows the query to cancel pending HTTP requests.
     */
    core::net::http::Request::Progress::Next progress_report(
            const core::net::http::Request::Progress& progress);

    /**
     * Hang onto the configuration information
     */
    Config::Ptr config_;

    /**
     * Thread-safe cancelled flag
     */
    std::atomic<bool> cancelled_;
};

}

#endif // API_CLIENT_H_

