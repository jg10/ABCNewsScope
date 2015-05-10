#include <boost/algorithm/string/trim.hpp>

#include <scope/localization.h>
#include <scope/query.h>

#include <unity/scopes/Annotation.h>
#include <unity/scopes/CategorisedResult.h>
#include <unity/scopes/CategoryRenderer.h>
#include <unity/scopes/QueryBase.h>
#include <unity/scopes/SearchReply.h>
#include <unity/scopes/Department.h>

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

    try {

        // Create the root department - top stories
        sc::Department::SPtr top_stories = sc::Department::create("", query(), "Top Stories");

        // Create the rest of the news categories departments
        sc::Department::SPtr just_in = sc::Department::create("51120", query(), "Just In");
        sc::Department::SPtr world = sc::Department::create("52278", query(), "World");
        sc::Department::SPtr australia = sc::Department::create("46182", query(), "Australia");
        sc::Department::SPtr business = sc::Department::create("51892", query(), "Business");
        sc::Department::SPtr entertainment = sc::Department::create("46800", query(), "Entertainment");
        sc::Department::SPtr sport = sc::Department::create("45924", query(), "Sport");
        sc::Department::SPtr the_drum = sc::Department::create("1054578", query(), "The Drum");

        top_stories->set_subdepartments({just_in, world, australia, business, entertainment, sport, the_drum});

        reply->register_departments(top_stories);

        // Start by getting information about the query
        const sc::CannedQuery &query(sc::SearchQueryBase::query());

        // Trim the query string of whitespace
        string query_string = alg::trim_copy(query.query_string());


        // the Client is the helper class that provides the results
        // without mixing APIs and scopes code.
        // Add your code to retreive xml, json, or any other kind of result
        // in the client.
        Client::NewsItemRes newslist;

        //cerr << query.department_id() << endl;

        if (!query.department_id().empty()) {
            // If a department is selected, get the news stories for its category
            newslist = client_.newsItems("*", query.department_id());
        } else {
            // otherwise, get the top stories
            newslist = client_.newsItems("*", "45910");
        }

        // Register a category for the top stories
        auto news_cat = reply->register_category("NewsItems", "", "",
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
