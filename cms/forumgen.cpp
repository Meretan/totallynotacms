#include <time.h>    // for ctime... remove it one day!

#include "database.hpp"
#include "dbforum.hpp"
#include "filters.hpp"

#include "forumgen.hpp"


ForumGenerator::~ForumGenerator()
{
    if(data)
        delete data;
    if(tree)
        delete tree;
}

bool ForumGenerator::GetArrayInfo(int &pg_count, bool &reverse) const
{
    return false;
}

void ForumGenerator::GetIndices(int &idx_file, int &idx_array) const
{
    idx_file = 0;
    idx_array = 0;
}

void ForumGenerator::SetHref(const ScriptVariable &href)
{
}

ScriptVariable ForumGenerator::Build()
{
    return "";
}

bool ForumGenerator::Next()
{
    return false;
}

ScriptVariable ForumGenerator::MakeCommentMap()
{
    return ScriptVariableInv();
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////


// this function must disappear one day when database gets
// a method to compose a textual date
// please note this is an almost-copy of a function with the same name
// from database.cpp
static void fill_date_from_unixtime(CommentData &data)
{
    if(data.unixtime > 0) {
        time_t tt = data.unixtime;
        data.date = asctime(gmtime(&tt));
            // XXX we need more flexibility here!
    } else {
        data.date =
            ScriptVariable("NODATE[") + ScriptNumber(data.id) + "]";
    }
}

static void convert_comment_node_to_data(const CommentNode *node,
                                         CommentData *cmt,
                                         const Database *database)
{
    FilterChainSet *filt = database->MakeFormatFilter(node->parser);
    const ScriptVector& hdr = node->parser.GetHeaders();
    cmt->id = node->id;
    cmt->parent = node->parent;
    int i;
    for(i = 0; i < hdr.Length()-1; i+=2) {
        if(hdr[i] == "id" || hdr[i] == "parent" ||
            hdr[i] == "encoding" || hdr[i] == "format")
        {
            // no storing
            continue;
        }
        if(hdr[i] == "unixtime") {
            bool ok = hdr[i+1].GetLongLong(cmt->unixtime, 10);
            if(!ok)
                cmt->unixtime = -1;
            continue;
        }
        if(hdr[i] == "user") {
            cmt->user = hdr[i+1];
            continue;
        }
        if(hdr[i] == "flags") {
            cmt->flags.Clear();
            cmt->flags = ScriptVector(hdr[i+1], ",", " \t\r\n");
            continue;
        }

        // the rest of fields are converted as Userdata
        ScriptVariable val = filt->ConvertUserdata(hdr[i+1]);

        if(hdr[i] == "date") {
            cmt->date = val;
            continue;
        }
        if(hdr[i] == "from") {
            cmt->from = val;
            continue;
        }
        if(hdr[i] == "title") {
            cmt->title = val;
            continue;
        }

        cmt->aux_params.AddItem(hdr[i]);
        cmt->aux_params.AddItem(val);
    }
    if(cmt->date == "")
        fill_date_from_unixtime(*cmt);

    cmt->text = filt->ConvertContent(node->parser.GetBody());

    delete filt;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

PlainForumGenerator::
PlainForumGenerator(const Database *db, ForumData *fd, CommentTree *ct)
    : ForumGenerator(db, fd, ct), pg_count(-1), cur_pg(1)
{
    next_cmt = data->reverse ? tree->GetMaxId() : 1;
}

void PlainForumGenerator::ScanPgCount()
{
    if(pg_count != -1)
        return;
    if(data->per_page <= 0) {
        pg_count = 1;
        return;
    }
    int count = 0;
    int max = tree->GetMaxId();
    int i;
    for(i = 1; i <= max; i++) {
        const CommentNode *comnode = tree->GetComment(i);
        if(!comnode)
            continue;
        if(!data->hidden_hold_place) {
            CommentData cmdata;
            convert_comment_node_to_data(comnode, &cmdata, the_database);
            if(cmdata.HasFlag("hidden"))
                continue;
        }
        count++;
    }
    pg_count = (count-1) / data->per_page + 1;
}

bool PlainForumGenerator::GetArrayInfo(int &pgcnt, bool &reverse) const
{
    if(data->per_page <= 0)
        return false;
    const_cast<PlainForumGenerator*>(this)->ScanPgCount();
    pgcnt = pg_count;
    reverse = data->reverse;
    return true;
}

void PlainForumGenerator::GetIndices(int &idx_file, int &idx_array) const
{
    if(cur_pg == 1 && !data->reverse) {
        idx_file = 0;
        idx_array = 1;
        return;
    }
#if 0    // nothing special in this special case
    if(cur_pg == 0 && data->reverse) {
        idx_file = 0;
        idx_array = 0;
        return;
    }
#endif
    idx_file = cur_pg;
    idx_array = cur_pg;
}

void PlainForumGenerator::SetHref(const ScriptVariable &href)
{
    cur_href = href;
}

ScriptVariable PlainForumGenerator::Build()
{
    ScriptVariable res;
    res = the_database->BuildGenericPart(data->top);

    int max = tree->GetMaxId();
    bool rev = data->reverse;

    ScanPgCount();
    if(pg_count < 2) {   // single-page case is special
        int i;
        for(i = rev ? max : 1; rev ? (i >= 1) : (i <= max); rev ? i-- : i++) {
            ScriptVariable c = BuildSingleComment(i);
            if(c.IsInvalid())
                continue;
            res += c;
        }
    } else
    if(cur_pg == 0) {    // the 'main' page is another special case
        if(!rev)
            return "ERROR: main comment page requested for non-reverse case";
        // yes, yes, it is reverse
        int count = 0;
        int i;
        for(i = max; i >= 1; i--) {
            ScriptVariable c = BuildSingleComment(i);
            if(c.IsInvalid())    // no such comment (deleted comment)
                continue;
            if(c == "" && !data->hidden_hold_place)  // hidden
                continue;
            res += c;
            count++;
            if(count >= data->per_page)
                break;
        }
    } else {     // okay, just one of the many pages
        ScriptVector cv;
        while(next_cmt <= max && cv.Length() < data->per_page) {
            ScriptVariable c = BuildSingleComment(next_cmt);
            next_cmt++;
            if(c.IsInvalid())
                continue;
            if(c == "" && !data->hidden_hold_place)  // hidden
                continue;
            cv.AddItem(c);
        }
        int len = cv.Length();
        int i;
        for(i = rev ? len-1 : 0; rev ? i >= 0 : i < len; rev ? i-- : i++)
            res += cv[i];
    }
    res += the_database->BuildGenericPart(data->bottom);
    return res;
}

bool PlainForumGenerator::Next()
{
    ScanPgCount();
    if(pg_count < 2)
        return false;
    if(cur_pg == 0)
        return false;
    cur_pg++;
    if(cur_pg <= pg_count)
        return true;
    // okay, numbered pages ended
    if(!data->reverse)
        return false;
    // for the reverse case, we still have to generate the main page
    cur_pg = 0;
    return true;
}

ScriptVariable PlainForumGenerator::BuildSingleComment(int cmt_id)
{
    const CommentNode *comnode = tree->GetComment(cmt_id);
    if(!comnode)
        return ScriptVariableInv();
    uris_of_pages[cmt_id] = cur_href;
    CommentData cmdata;
    convert_comment_node_to_data(comnode, &cmdata, the_database);
    bool hidden = cmdata.HasFlag("hidden");
    if(hidden)
        return "";
    cmdata.the_tree_aux_params = &(tree->GetAuxParams());
    cmdata.page_of_parent_uri =
        cmdata.parent > 0 && cmdata.parent < uris_of_pages.Length()
            ? uris_of_pages[cmdata.parent] : "";
    return the_database->BuildCommentPart(cmdata, data->comment_templ);
}

#if 0
bool PlainForumGenerator::
NextPage(const ScriptVariable &href, class ScriptVariable &s)
{
    if(!tree)       // this means all is done already
        return false;

    if(tree->GetMaxId() == 0) {
        s = the_database->BuildGenericPart(data->no_comments_templ);
        delete tree;
        tree = 0;
        return true;
    }

    s = the_database->BuildGenericPart(data->top);

    int max = tree->GetMaxId();
    bool rev = data->reverse;
    int count = 0;       // how many comments are already at this page
    for(; rev ? (next_cmt > 0) : (next_cmt <= max);
        rev ? next_cmt-- : next_cmt++)
    {
        const CommentNode *comnode = tree->GetComment(next_cmt);
        if(!comnode)
            continue;
        uris_of_pages[next_cmt] = href;
        CommentData cmdata;
        convert_comment_node_to_data(comnode, &cmdata, the_database);
        bool hidden = cmdata.HasFlag("hidden");
        if(hidden && !data->hidden_hold_place)
            continue;
        if(!hidden) {
            cmdata.the_tree_aux_params = &(tree->GetAuxParams());
            cmdata.page_of_parent_uri =
                cmdata.parent > 0 && cmdata.parent < uris_of_pages.Length()
                    ? uris_of_pages[cmdata.parent] : "";
            s += the_database->BuildCommentPart(cmdata, data->comment_templ);
        }
        count++;
        if(data->per_page > 0 && count >= data->per_page)
            break;
    }
    s += the_database->BuildGenericPart(data->bottom);
    if(rev ? (next_cmt <= 0) : (next_cmt > max)) {
        delete tree;
        tree = 0;
    }
    return true;
}
#endif

ScriptVariable PlainForumGenerator::MakeCommentMap()
{
    ScriptVariable res("");
    int i;
    int ul = uris_of_pages.Length();
    for(i = 0; i < ul; i++) {
        if(uris_of_pages[i].IsInvalid() || uris_of_pages[i] == "")
            continue;
        res += ScriptNumber(i);
        res += " ";
        res += uris_of_pages[i];
        res += "\n";
    }
    return res;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

static void int_swap(int *a, int *b)
{
    int t = *a;
    *a = *b;
    *b = t;
}

static int bubble_sort_child_array(int *arr)
{
    if(!*arr || !arr[1])
        return *arr ? 1 : 0;
    bool changed = false;
    int i;
        // we need to compute the length, so why don't we use the pass fully?
    for(i = 1; arr[i]; i++) {
        if(arr[i] < arr[i-1]) {
            int_swap(arr+i, arr+(i-1));
            changed = true;
        }
    }
    int len = i;
    if(!changed)
        return len;
    int restlen;
    for(restlen = len; restlen > 1; restlen--) {
        changed = false;
        for(i = 1; i < restlen; i++) {
            if(arr[i] < arr[i-1]) {
                int_swap(arr+i, arr+(i-1));
                changed = true;
            }
        }
        if(!changed)
            return len;
    }
    return len;
}


static ScriptVariable make_comment_subtree(const CommentNode *comnode,
                                           const CommentTree *tree,
                                           const ForumData *fdata,
                                           const Database *database)
{
    CommentData data;
    convert_comment_node_to_data(comnode, &data, database);
    if(data.HasFlag("hidden"))
        return "";
    data.the_tree_aux_params = &(tree->GetAuxParams());

    ScriptVariable res;

    res += database->BuildCommentPart(data, fdata->comment_templ);
    int *chl = comnode->children;
    if(chl) {
        res += database->BuildGenericPart(fdata->indent);
        int len = bubble_sort_child_array(chl);
        int i;
        for(i = 0; i < len; i++) {
            res += make_comment_subtree(tree->GetComment(chl[i]),
                                        tree, fdata, database);
        }
        res += database->BuildGenericPart(fdata->unindent);
    }
    res += database->BuildCommentPart(data, fdata->tail);

    return res;
}

ScriptVariable SingleTreeForumGenerator::Build()
{
    if(!tree)       // this means all is done already
        return "";

    ScriptVariable s;
    if(tree->GetMaxId() == 0) {
        s = the_database->BuildGenericPart(data->no_comments_templ);
    } else {
        s = the_database->BuildGenericPart(data->top);
        int *top_ch = tree->GetComment(0)->children;
        int len = bubble_sort_child_array(top_ch);
        bool rev = data->reverse;
        int i;
        for(i = rev ? len-1 : 0; rev ? (i >= 0) : (i < len); rev ? i-- : i++) {
            s += make_comment_subtree(tree->GetComment(top_ch[i]),
                                      tree, data, the_database);
        }
        s += the_database->BuildGenericPart(data->bottom);
    }
    delete tree;
    tree = 0;
    return s;
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

ScriptVariable ForestForumGenerator::Build()
{
    return ""; // XXX stub
}

//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////

ScriptVariable ErrorForumGenerator::Build()
{
    if(!msg.IsInvalid())       // this means all is done already
        return "";
    ScriptVariable s = msg;
    msg.Invalidate();
    return s;
}
