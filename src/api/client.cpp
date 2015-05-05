#include <api/client.h>

#include <core/net/error.h>
#include <core/net/http/client.h>
#include <core/net/http/content_type.h>
#include <core/net/http/response.h>

namespace http = core::net::http;
namespace net = core::net;

using namespace api;
using namespace std;

Client::Client(Config::Ptr config) :
    config_(config), cancelled_(false) {
}

namespace {
static QString readText(QXmlStreamReader& xml)
{
    xml.readNext();

    if (xml.tokenType() != QXmlStreamReader::Characters) {
        return QString();
    }

    return xml.text().toString();
}

static void parseItem(Client::NewsItemRes& newsItemRes, QXmlStreamReader& xml)
{
    //Create a news item object
    Client::NewsItem newsItem;

    while (!xml.atEnd() && !(xml.isEndElement() && xml.name() == "item"))
    {

        if (xml.isStartElement()) {


            //newsItem.image += readText(xml).toStdString();
            if (xml.name() == "title") {
                newsItem.title = readText(xml).toStdString();
            }
            if (xml.name() == "link") {
                newsItem.link = readText(xml).toStdString();
            }
            if (xml.name() == "description") {
                newsItem.description = readText(xml).toStdString();
                //newsItem.image = "http://www.abc.net.au/news/image/6439296-16x9-2150x1210.jpg";
            }
            if (xml.name() == "content") {

                newsItem.image = xml.attributes().value("url").toString().toStdString();
                //newsItem.image = "http://www.abc.net.au/news/image/6439296-16x9-2150x1210.jpg";
            }
        }
        xml.readNext();
    }
    // Add the news item to our list of news items
    newsItemRes.newsItems.emplace_back(newsItem);
}


}

void Client::get(const net::Uri::Path &path,
                 const net::Uri::QueryParameters &parameters, QXmlStreamReader &reader) {
    // Create a new HTTP client
    auto client = http::make_client();

    // Start building the request configuration
    http::Request::Configuration configuration;

    // Build the URI from its components
    net::Uri uri = net::make_uri(config_->apiroot, path, parameters);
    configuration.uri = client->uri_to_string(uri);

    // Give out a user agent string
    configuration.header.add("User-Agent", config_->user_agent);

    // Build a HTTP request object from our configuration
    auto request = client->head(configuration);

    try {
        // Synchronously make the HTTP request
        // We bind the cancellable callback to #progress_report
        auto response = request->execute(
                    bind(&Client::progress_report, this, placeholders::_1));

        // Check that we got a sensible HTTP status code
        if (response.status != http::Status::ok) {
            throw domain_error(response.body);
        }
        // Parse the Xml from the response
        reader.addData(response.body.c_str());
    } catch (net::Error &) {
    }
}

Client::NewsItemRes Client::newsItems(const string& query, const string &category) {
    // This is the method that we will call from the Query class.
    // It connects to an HTTP source and returns the results.

    // In this case we are going to retrieve XML data.
    QXmlStreamReader root;

    // Build a URI and get the contents.
    // The fist parameter forms the path part of the URI.
    // The second parameter forms the CGI parameters.
    get(
    { category, "rss.xml" },
    { { }
      , { "mode", "xml" }
                },
                root);


    NewsItemRes result;

    while (!root.atEnd() && !root.hasError()) {
        QXmlStreamReader::TokenType token = root.readNext();

        /* If token is just StartDocument, we'll go to next.*/
        if (token == QXmlStreamReader::StartDocument) {
            continue;
        }

        /* If token is StartElement, we'll see if we can read it.*/
        if (token == QXmlStreamReader::StartElement) {
            if (root.name() == "item") {
                parseItem(result, root);
            } else {
                root.readNext();
            }
        }
    }

    if (root.hasError()) {
        throw domain_error(root.errorString().toStdString());
    }
    return result;
}



http::Request::Progress::Next Client::progress_report(
        const http::Request::Progress&) {

    return cancelled_ ?
                http::Request::Progress::Next::abort_operation :
                http::Request::Progress::Next::continue_operation;
}

void Client::cancel() {
    cancelled_ = true;
}

Config::Ptr Client::config() {
    return config_;
}

