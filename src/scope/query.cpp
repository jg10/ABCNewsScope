#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>

#include <iomanip>
#include <sstream>

namespace sc = unity::scopes;
namespace alg = boost::algorithm;

using namespace std;
using namespace api;
using namespace scope;


/**
 * Define the layout for the news item results
 *
 * The icon size is small, and ask for the card layout
 * itself to be horizontal. I.e. the text will be placed
 * next to the image.
 */
const static string NEWS_TEMPLATE =
        R"(
{
        "schema-version": 1,
        "template": {
        "category-layout": "grid",
        "card-layout": "horizontal",
        "card-size": "small"
        },
        "components": {
        "title": "title",
        "art" : {
        "field": "art"
        },
        "subtitle": "subtitle"
        }
        }
        )";


Query::Query(const sc::CannedQuery &query, const sc::SearchMetadata &metadata,
             Config::Ptr config) :
    sc::SearchQueryBase(query, metadata), client_(config) {
}

void Query::cancelled() {
    client_.cancel();
}


void Query::run(sc::SearchReplyProxy const& reply) {
    cerr << "About to run initscope" << endl;
    initScope();

    try {
        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());

        // Get the correct URI part for the chosen category
        std:string catnum, catname;

        switch (category) {
            case 0:
                catnum = "45910";
                catname = "Top Stories";
                break;
            case 1:
                catnum = "51120";
                catname = "Just In";
                break;
            case 2:
                catnum = "52278";
                catname = "World";
                break;
            case 3:
                catnum = "46182";
                catname = "Australia";
                break;
            case 4:
                catnum = "51892";
                catname = "Business";
                break;
            case 5:
                catnum = "46800";
                catname = "Entertainment";
                break;
            case 6:
                catnum = "45924";
                catname = "Sport";
                break;
            case 7:
                catnum = "1054578";
                catname = "The Drum";
                break;
            default:
                catnum = "45910";
                catname = "Top Stories";
        }

        // the Client is the helper class that provides the results
        // without mixing APIs and scopes code.
        // Add your code to retreive xml, json, or any other kind of result
        // in the client.
        Client::NewsItemRes newslist;

        if (query_string.empty()) {
            // If the string is empty, get the unfiltered top stories
            newslist = client_.newsItems("*", catnum);
        } else {
            // otherwise, get the top stories for the search string
            newslist = client_.newsItems(query_string, catnum);
        }

        // Register a category for the top stories
        auto news_cat = reply->register_category("NewsItems", catname, "",
                                                     sc::CategoryRenderer(NEWS_TEMPLATE));
        for (const auto &newsitem : newslist.newsItems) {

            // Create a single result for the news category
            sc::CategorisedResult res(news_cat);

            // We must have a URI
            res.set_uri(newsitem.link);

            // Build up the description for the news item
            res.set_title(newsitem.title);

            // Set the rest of the attributes, art, description, etc
            res.set_art(newsitem.image);
            res["subtitle"] = newsitem.description;
            res["description"] = newsitem.description;
            res["link"] = newsitem.link;

            // Push the result
            if (!reply->push(res)) {
                // If we fail to push, it means the query has been cancelled.
                // So don't continue;
                return;
            }
        }

    } catch (domain_error &e) {
        // Handle exceptions being thrown by the client API
        cerr << e.what() << endl;
        reply->error(current_exception());
    }
}

void Query::initScope()
{
    cerr << "run settings()" << endl;
    unity::scopes::VariantMap config = settings();  // The settings method is provided by the base class
    cerr << "after settings()" << endl;
    if (config.empty())
        cerr << "CONFIG EMPTY!" << endl;

    category = config["category"].get_int();
    cerr << "category: " << category << endl;

}

