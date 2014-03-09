#include <hre/config.h>

#include <limits.h>
#include <stdlib.h>
#include <time.h>

#include <dm/dm.h>
#include <hre/stringindex.h>
#include <hre/user.h>
#include <ltsmin-lib/ltsmin-standard.h>
#include <pins-lib/pins.h>
#include <pins-lib/pins2pins-por.h>
#include <pins-lib/pins-util.h>
#include <util-lib/bitmultiset.h>
#include <util-lib/dfs-stack.h>
#include <util-lib/util.h>

int POR_WEAK = 0; //extern

static int NO_COMMUTES = 0;
static int NO_HEUR = 0;
static int NO_BEAM = 0;
static int NO_HEUR_BEAM = 0;
static int NO_DNA = 0;
static int NO_NES = 0;
static int NO_NDS = 0;
static int NO_MDS = 0;
static int NO_MCNDS = 0;
static int NO_MC = 0;
static int NO_DYN_VIS = 0;
static int NO_V = 0;
static int NO_L12 = 0;
static int PREFER_NDS = 0;
static int RANDOM = 0;
static const char *algorithm = "heur";

static int SAFETY = 0;

typedef enum {
    POR_NONE,
    POR_HEUR,
    POR_DEL,
    POR_SCC,
    POR_SCC1,
} por_alg_t;

static por_alg_t alg = -1;

static si_map_entry por_algorithm[]={
    {"none",    POR_NONE},
    {"",        POR_HEUR},
    {"heur",    POR_HEUR},
    {"del",     POR_DEL},
    {"scc",     POR_SCC},
    {"scc1",    POR_SCC1},
    {NULL, 0}
};

static void
por_popt (poptContext con, enum poptCallbackReason reason,
          const struct poptOption *opt, const char *arg, void *data)
{
    (void)con; (void)data;
    switch (reason) {
    case POPT_CALLBACK_REASON_PRE: break;
    case POPT_CALLBACK_REASON_POST: break;
    case POPT_CALLBACK_REASON_OPTION:
        if (opt->shortName != 'p') return;
        if (arg == NULL) arg = "";
        int num = linear_search (por_algorithm, arg);
        if (num < 0) {
            Warning (error, "unknown POR algorithm %s", arg);
            HREprintUsage();
            HREexit(LTSMIN_EXIT_FAILURE);
        }
        if ((alg = num) != POR_NONE)
            PINS_POR = PINS_POR_ON;
        return;
    }
    Abort("unexpected call to por_popt");
}

struct poptOption por_options[]={
    {NULL, 0, POPT_ARG_CALLBACK, (void *)por_popt, 0, NULL, NULL},
    { "por", 'p', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL | POPT_ARGFLAG_SHOW_DEFAULT,
      &algorithm, 0, "enable partial order reduction", "<|heur|del|scc>" },

    /* HIDDEN OPTIONS FOR EXPERIMENTATION */

    { "check" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &PINS_POR , PINS_POR_CHECK , "verify partial order reduction peristent sets" , NULL },
    { "no-dna" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_DNA , 1 , "without DNA" , NULL },
    { "no-commutes" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_COMMUTES , 1 , "without commutes (for left-accordance)" , NULL },
    { "no-nes" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_NES , 1 , "without NES" , NULL },
    { "no-heur" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_HEUR , 1 , "without heuristic" , NULL },
    { "no-beam" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_BEAM , 1 , "without beam search" , NULL },
    { "no-heur-beam" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_HEUR_BEAM , 1 , "without heuristic / beam search" , NULL },
    { "no-mds" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_MDS , 1 , "without MDS" , NULL },
    { "no-nds" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_NDS , 1 , "without NDS (for dynamic label info)" , NULL },
    { "no-mc" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_MC , 1 , "without MC" , NULL },
    { "no-mcnds" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_MCNDS , 1 , "Do not create NESs from MC and NDS" , NULL },
    { "no-dynamic-labels" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_DYN_VIS , 1 , "without dynamic visibility" , NULL },
    { "no-V" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_V , 1 , "without V proviso, instead use Peled's visibility proviso, or V'     " , NULL },
    { "no-L12" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &NO_L12 , 1 , "without L1/L2 proviso, instead use Peled's cycle proviso, or L2'   " , NULL },
    { "prefer-nds" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &PREFER_NDS , 1 , "prefer MC+NDS over NES" , NULL },
    { "por-random" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &RANDOM , 1 , "randomize enabled and NES selection" , NULL },
    { "weak" , 0, POPT_ARG_VAL | POPT_ARGFLAG_DOC_HIDDEN , &POR_WEAK , 1 , "Weak stubborn set theory" , NULL },
    POPT_TABLEEND
};

typedef enum {
    VISIBLE,            // group or label NES/NDS
    VISIBLE_GROUP,      // group
    VISIBLE_NES,        // label NES
    VISIBLE_NDS,        // label NDS
    VISIBLE_COUNT
} visible_t;

static inline int
is_visible (por_context* ctx, int group)
{
    return bms_has(ctx->visible, VISIBLE, group);
}

// number of necessary sets (halves if MC is absent, because no NDSs then)
static inline int
NS_SIZE (por_context* ctx)
{
    return NO_MCNDS ? ctx->nguards : ctx->nguards << 1;
}

/**
 * Initialize the structures to record visible groups.
 */
static inline void
init_visible_labels (por_context* ctx)
{
    if (ctx->visible != NULL) return;
    NO_DYN_VIS |= NO_V;

    model_t model = ctx->parent;
    int groups = dm_nrows (GBgetDMInfo(model));
    ctx->visible = bms_create (groups, VISIBLE_COUNT);

    for (int i = 0; i < groups; i++) {
        if (!ctx->group_visibility[i]) continue;
        bms_push_new (ctx->visible, VISIBLE, i);
        bms_push_new (ctx->visible, VISIBLE_GROUP, i);
    }

    for (int i = 0; i < ctx->nlabels; i++) {
        if (!ctx->label_visibility[i]) continue;

        for (int j = 0; j < ctx->label_nes[i]->count; j++) {
            int group = ctx->label_nes[i]->data[j];
            bms_push_new (ctx->visible, VISIBLE_NES, group);
            bms_push_new (ctx->visible, VISIBLE, group);
        }
        for (int j = 0; j < ctx->label_nds[i]->count; j++) {
            int group = ctx->label_nds[i]->data[j];
            bms_push_new (ctx->visible, VISIBLE_NDS, group);
            bms_push_new (ctx->visible, VISIBLE, group);
        }
    }
    int vgroups = bms_count(ctx->visible, VISIBLE_GROUP);
    if (!NO_DYN_VIS && vgroups > 0 && vgroups != bms_count(ctx->visible, VISIBLE)) {
        Print1 (info, "Turning off dynamic visibility in presence of visible groups");
        NO_DYN_VIS = 1;
    }
    SAFETY = bms_count(ctx->visible, VISIBLE) != 0;
}

static void
por_init_transitions (model_t model, por_context *ctx, int *src)
{
    init_visible_labels (ctx);

    // fill guard status, request all guard values
    GBgetStateLabelsGroup (model, GB_SL_GUARDS, src, ctx->label_status);

    ctx->visible_enabled = 0;
    ctx->visible_nes_enabled = 0;
    ctx->visible_nds_enabled = 0;
    ctx->enabled_list->count = 0;
    // fill group status and score
    for (int i = 0; i < ctx->ngroups; i++) {
        ctx->group_status[i] = GS_ENABLED; // reset
        // mark groups as disabled
        for (int j = 0; j < ctx->group2guard[i]->count; j++) {
            int guard = ctx->group2guard[i]->data[j];
            if (ctx->label_status[guard] == 0) {
                ctx->group_status[i] = GS_DISABLED;
                break;
            }
        }
        // set group score
        if (ctx->group_status[i] == GS_ENABLED) {
            ctx->enabled_list->data[ctx->enabled_list->count++] = i;
            ctx->visible_enabled += is_visible (ctx, i);
            ctx->visible_nes_enabled += ctx->visible->set[i] & (1 << VISIBLE_NES);
            ctx->visible_nds_enabled += ctx->visible->set[i] & (1 << VISIBLE_NDS);
        }
    }
}

static inline void
incr_ns_update (por_context *ctx, int group, int new_group_score)
{
    int oldgroup_score = ctx->group_score[group];
    ctx->group_score[group] = new_group_score;
    if (oldgroup_score == new_group_score) return;

    for (int i = 0; i < ctx->group2ns[group]->count; i++) {
        int ns = ctx->group2ns[group]->data[i];
        ctx->nes_score[ns] += new_group_score - oldgroup_score;
    }
}

static void
por_transition_costs (por_context *ctx)
{
    if (NO_HEUR) return;

    // set score for enable transitions
    if (PINS_LTL || SAFETY) {
        int vis;
        for (int i = 0; i < ctx->ngroups; i++) {
            int new_score;
            if (ctx->group_status[i] == GS_DISABLED) {
                new_score = 1;
            } else if ((vis = ctx->visible->set[i])) {
                if (NO_V) {
                    new_score = ctx->enabled_list->count * ctx->ngroups;
                } else {
                    if (NO_DYN_VIS || (vis & ((1 << VISIBLE_NES) | (1 << VISIBLE_NDS)))) {
                        new_score = ctx->visible_enabled * ctx->ngroups +
                                bms_count(ctx->visible, VISIBLE) - ctx->visible_enabled;
                    } else if (vis & (1 << VISIBLE_NES)) {
                        new_score = ctx->visible_nds_enabled * ctx->ngroups +
                                bms_count(ctx->visible, VISIBLE_NDS) - ctx->visible_nds_enabled;
                    } else { // VISIBLE_NDS:
                        new_score = ctx->visible_nes_enabled * ctx->ngroups +
                                bms_count(ctx->visible, VISIBLE_NES) - ctx->visible_nes_enabled;
                    }
                }
            } else {
                new_score = ctx->ngroups;
            }
            incr_ns_update (ctx, i, new_score);
        }
    } else {
        for (int i = 0; i < ctx->ngroups; i++) {
            int enabled = ctx->group_status[i] == GS_ENABLED;
            incr_ns_update (ctx, i, enabled ? ctx->ngroups : 1);
        }
    }
}

/**
 * The function update_score is called whenever a new group is added to the stubborn set
 * It takes care of updating the heuristic function for the nes based on the new group selection
 */
static inline void
update_ns_scores (por_context* ctx, search_context *s, int group)
{
    if (NO_HEUR) return;

    // change the heuristic function according to selected group
    for(int k=0 ; k< ctx->group2ns[group]->count; k++) {
        int ns = ctx->group2ns[group]->data[k];
        // note: this selects some nesses that aren't used, but take the extra work
        // instead of accessing the guards to verify the work
        s->nes_score[ns] -= ctx->group_score[group];
    }
}

/**
 * Mark a group selected, update counters and NS scores.
 */
static inline void
select_group (por_context* ctx, int group)
{
    // get current search context
    search_context *s = &ctx->search[ ctx->search_order[0] ];

    // already selected?
    if (s->emit_status[group] & ES_SELECTED) {
        Printf (debug, "(%d), ", group);
        return;
    }

    s->emit_status[group] |= ES_SELECTED;

    update_ns_scores (ctx, s, group);

    // and add to work array and update counts
    int visible = is_visible (ctx, group);
    if (ctx->group_status[group] & GS_DISABLED) {
        s->work[s->work_disabled--] = group;
    } else {
        s->work[s->work_enabled++] = group;
        s->ve_selected += visible;
        s->enabled_selected++;
    }
    s->visibles_selected += visible;
    Printf (debug, "%d, ", group);
}

static inline void
select_one_invisible (por_context* ctx)
{
    search_context *s = &ctx->search[ctx->search_order[0]];
    HREassert (s->has_key);

    // Valmari's L1 proviso requires one invisible transition (to include quiescent runs)
    for(int i=0; i<ctx->enabled_list->count; i++) {
        int group = ctx->enabled_list->data[i];
        if (!is_visible(ctx, group)) {
            select_group (ctx, group);
            Printf (debug, "Added extra invisible: %d\n", group);
            if (POR_WEAK && s->has_key == -1) { // make it a key as well
                for (int g = 0; g < ctx->not_accords[group]->count; g++) {
                    int gg = ctx->not_accords[group]->data[g];
                    select_group (ctx, gg);
                }
                Printf (debug, "Made added invisible also key: %d\n", group);
            }
            return;
        }
    }
    HREassert (false, "Called select_one_invisible without enabled invisible transitions.");
}

static inline void
select_all_visible (por_context* ctx, int set)
{
    // Valmari's V-proviso: implicate all visible groups
   for (int i = 0; i < bms_count(ctx->visible, set); i++) {
       int group = ctx->visible->lists[set]->data[i];
       select_group (ctx, group);
   }
}

/**
 * These functions emits the persistent set with cycle proviso
 * To ensure this proviso extra communication with the algorithm is required.
 * The algorithm annotates each transition with the por_proviso flag.
 * For ltl, all selected transition groups in the persistent set must
 * have this por_proviso flag set to true, otherwise enabled(s) will be returned.
 * For safety, the proviso needs to hold for at least on emitted state.
 * The client may (should) set the proviso always to true for deadlocks.
 */

typedef struct proviso_hook_context {
    TransitionCB    cb;
    void           *user_context;
    int             por_proviso_true_cnt;
    int             por_proviso_false_cnt;
    int             force_proviso_true;     // feedback to algorithm that proviso already holds
} proviso_hook_context_t;

void hook_cb (void *context, transition_info_t *ti, int *dst, int *cpy) {
    proviso_hook_context_t* infoctx = (proviso_hook_context_t*)context;
    transition_info_t ti_new = GB_TI (ti->labels, ti->group);
    ti_new.por_proviso = infoctx->force_proviso_true;
    infoctx->cb(infoctx->user_context, &ti_new, dst, cpy);
    // catch transition info status
    if (infoctx->force_proviso_true || ti_new.por_proviso) {
        infoctx->por_proviso_true_cnt++;
    } else {
        infoctx->por_proviso_false_cnt++;
    }
}

static inline int
emit_new_selected (por_context *ctx, proviso_hook_context_t *provctx, int *src)
{
    search_context *s = &ctx->search[ctx->search_order[0]];
    int c = 0;
    for (int z = 0; z < ctx->enabled_list->count; z++) {
        int i = ctx->enabled_list->data[z];

        if (por_is_stubborn(ctx,i) && !(s->emit_status[i] & ES_EMITTED)) {
            s->emit_status[i] |= ES_EMITTED;
            c += GBgetTransitionsLong (ctx->parent, i, src, hook_cb, provctx);
        }
    }
    return c;
}

/**
 * Visible transtions:      Tv
 * Invisible transtions:    Ti = T \ Tv
 * Stubborn set:            Ts
 * Keys in stubborn set:    Tk
 *
 * V  = Tv n Ts != {}     ==>  Tv C Ts
 * L1 = Ti n en(s) != {}  ==>  Tk n Ti != {}
 * L2 = s closes cycle    ==>  Tv C Ts
 *
 * Whether s closes a cycle is determined by the search algorithm, which may
 * employ DFS with the condition s in stack, or a more complicated search
 * algorithm such as the color proviso.
 *
 * Premature check whether L1 and L2 hold, i.e. before ignoring condition is
 * known (the premise of L2).
 * For safety (!PINS_LTL), we limit the proviso to L2. For details see
 * implementation notes in the header.
 */
static inline int
check_L1_L2_proviso (por_context* ctx)
{
    search_context *s = &ctx->search[ctx->search_order[0]];
    return s->visibles_selected == bms_count(ctx->visible,VISIBLE) && // all visible selected: satisfies (the conclusion of) L2
     (!PINS_LTL || // safety!
      ctx->visible_enabled == ctx->enabled_list->count ||   // no invisible is enabled: satisfies (the premise of) L1
      s->ve_selected != s->enabled_selected) &&             // one invisible enabled selected: satisfies (the conclusion of) L1
      (!POR_WEAK || s->has_key == 1);                       // weak requires key invisible
}

/**
 * Based on the heuristic and find the cheapest NS (NES/NDS) for a disabled
 * group.
 */
static inline int
find_cheapest_ns (por_context* ctx, search_context *s, int group)
{
    int n_guards = ctx->nguards;
    int count = ctx->group_has[group]->count;
    HREassert (count > 0, "Group %d has no NES", group);

    if (NO_HEUR) {
        if (RANDOM)
            return ctx->group_has[group]->data[ clock() % count ];
        return ctx->group_has[group]->data[ 0 ];
    }

    // for a disabled transition we need to add the necessary set
    // lookup which set has the lowest score on the heuristic function h(x)
    int selected_ns = -1;
    int selected_score = INT32_MAX;
    for (int k = 0; k < count; k++) {

        int ns = ctx->group_has[group]->data[ k ];

        // check the score by the heuristic function h(x)
        if (s->nes_score[ns] < selected_score) {
            // check guard status for ns (nes for disabled and nds for enabled):
            if ((ns < n_guards && (ctx->label_status[ns] == 0))  ||
                (ns >= n_guards && (ctx->label_status[ns-n_guards] != 0)) ) {

                // make this the current best
                selected_ns = ns;
                selected_score = s->nes_score[ns];
                // if score is 0 it can't improve, break the loop
                if (selected_score == 0) return selected_ns;
            }
        }
    }
    return selected_ns;
}

static void
beam_add_dna_for_enabled (por_context *ctx, int group)
{
    ci_list **accords = POR_WEAK ? ctx->not_left_accords : ctx->not_accords;
    for (int j = 0; j < accords[group]->count; j++) {
        int dependent_group = accords[group]->data[j];
        select_group (ctx, dependent_group);
    }
}

/**
 * Sorts BEAM search contexts.
 */
static bool
beam_sort (por_context *ctx)
{
    search_context *s = &ctx->search[ ctx->search_order[0] ];

    // if it can't move, we found the best score
    if (ctx->beam_used > 1 && s->score > ctx->search[ctx->search_order[1]].score) {
        // bubble current context down the search, continue with other context
        // this is known by the above conditions
        ctx->search_order[0] = ctx->search_order[1];
        // continue with 2
        int bubble = 2;
        while(bubble < ctx->beam_used && s->score >= ctx->search[ctx->search_order[bubble]].score) {
            ctx->search_order[bubble-1] = ctx->search_order[bubble];
            bubble++;
        }
        bubble--;

        if (bubble < ctx->beam_used)
            ctx->search_order[bubble] = s->idx;
    } else if (s->work_disabled == ctx->ngroups) {
        Printf (debug, "bailing out, no disabled work\n");
        return false;
    }
    return true;
}

/**
 * Analyze NS is the function called to find the smallest persistent set
 * It builds stubborn sets in multiple search contexts, by using a beam
 * search it switches search context each time the score of the current context
 * (based on heuristic function h(x) (nes_score)) isn't the lowest score anymore
 */
static inline void
beam_search (por_context* ctx)
{
    // if no search context is used, there are no transitions, nothing to analyze
    if (ctx->beam_used == 0) return;

    // infinite loop searching in multiple context, will bail out
    // when persistent set is found
    do {
        // start searching in multiple contexts
        // the search order is a sorted array based on the score of the search context
        // start with the context in search_order[0] = best current score
        search_context *s = &ctx->search[ ctx->search_order[0] ];

        // while there are disabled transitions:
        while (s->work_enabled == 0 && s->work_disabled < ctx->ngroups) {
            // one disabled transition less, increase the count (work_disabled = n -> no disabled transitions)
            s->work_disabled++;
            int current_group = s->work[s->work_disabled];

            // bail out if already ready
            if (s->emit_status[current_group] & ES_READY) continue;

            // mark as selected and ready
            s->emit_status[current_group] |= ES_SELECTED | ES_READY;

            Printf (debug, "BEAM-%d investigating group %d (disabled) --> ", s->idx, current_group);

            int selected_ns = find_cheapest_ns (ctx, s, current_group);

            // add the selected nes to work
            for(int k=0; k < ctx->ns[selected_ns]->count; k++) {
                int group = ctx->ns[selected_ns]->data[k];
                select_group (ctx, group);
            }
            Printf (debug, " (ns %d (%s))\n", selected_ns % ctx->nguards,
                    selected_ns < ctx->nguards ? "disabled" : "enabled");
        }

        // if the current search context has enabled transitions, handle all of them
        while (s->work_enabled > 0) {
            // one less enabled transition (work_enabled = 0 -> no enabled transitions)
            s->work_enabled--;
            int current_group = s->work[s->work_enabled];
            Printf (debug, "BEAM-%d investigating group %d (enabled) --> ", s->idx, current_group);

            // select and mark as ready
            s->emit_status[current_group] |= ES_SELECTED | ES_READY;

            // init search
            if (!s->initialized) {
                if (!NO_HEUR)
                    memcpy(s->nes_score, ctx->nes_score, sizeof(int[NS_SIZE(ctx)]));
                update_ns_scores (ctx, s, current_group);
                s->initialized = 1;
            } else {
                // update the search score
                s->score += 1;
            }

            // V proviso only for LTL
            if ((PINS_LTL || SAFETY) && is_visible(ctx, current_group)) {
                if (NO_V) { // Use Peled's stronger visibility proviso:
                    s->score += ctx->ngroups; // selects all groups in this search context
                } else if (NO_DYN_VIS) {
                    select_all_visible (ctx, VISIBLE);
                } else {
                    if (ctx->visible->set[current_group] & (1 << VISIBLE_NES))
                        select_all_visible (ctx, VISIBLE_NDS);
                    if (ctx->visible->set[current_group] & (1 << VISIBLE_NDS))
                        select_all_visible (ctx, VISIBLE_NES);
                }
            }

            // quit the current search when emit_limit is reached
            // this block is just to skip useless work, everything is emitted anyway
            if (s->score >= ctx->enabled_list->count) {
                s->work_enabled = 0;
                s->work_disabled = ctx->ngroups;
                Printf (debug, " (quitting |ss|=|en|)\n");
                continue;
            }

            // push all dependent unselected ctx->ngroups
            beam_add_dna_for_enabled (ctx, current_group);
            Printf (debug, "\n");
        }
    } while (beam_sort(ctx));
}

static inline int
beam_emit (por_context* ctx, int* src, TransitionCB cb, void* uctx)
{
    // if no enabled transitions, return directly
    if (ctx->beam_used == 0) return 0;
    // selected in winning search context
    search_context *s = &ctx->search[ctx->search_order[0]];
    int emitted = 0;
    // if the score is larger then the number of enabled transitions, emit all
    if (s->score >= ctx->enabled_list->count) {
        // return all enabled
        proviso_hook_context_t provctx = {cb, uctx, 0, 0, 1};
        for (int z = 0; z < ctx->enabled_list->count; z++) {
            int i = ctx->enabled_list->data[z];
            emitted += GBgetTransitionsLong (ctx->parent, i, src, hook_cb, &provctx);
        }
    } else if (!PINS_LTL && !SAFETY) { // deadlocks are easy:
        proviso_hook_context_t provctx = {cb, uctx, 0, 0, 1};
        emitted = emit_new_selected (ctx, &provctx, src);
    } else { // otherwise enforce that all por_proviso flags are true
        proviso_hook_context_t provctx = {cb, uctx, 0, 0, 0};

        provctx.force_proviso_true = !NO_L12 && !NO_V && check_L1_L2_proviso(ctx);
        emitted = emit_new_selected (ctx, &provctx, src);

        // emit more if we need to fulfill a liveness / safety proviso
        if ( ( PINS_LTL && provctx.por_proviso_false_cnt != 0) ||
             (!PINS_LTL && provctx.por_proviso_true_cnt  == 0) ) {

            if (!NO_L12 && !NO_V) {
                // enforce L2 (include all visible transitions)
                select_all_visible (ctx, VISIBLE);
                ctx->beam_used = 1; // fix to current (partly emitted) search ctx
                beam_search (ctx);

                // enforce L1 (one invisible transition)
                // not to be worried about when using Peled's visibility
                provctx.force_proviso_true = check_L1_L2_proviso(ctx);
                if (PINS_LTL && !provctx.force_proviso_true) {
                    select_one_invisible (ctx);
                    beam_search (ctx);
                }
            } else {
                s->score = ctx->enabled_list->count; // force all enabled
            }
            emitted += emit_new_selected (ctx, &provctx, src);
        }
    }
    return emitted;
}

static inline void
beam_ensure_key (por_context* ctx)
{
    // if no enabled transitions, return directly
    search_context *s = &ctx->search[ctx->search_order[0]];
    int enabled = ctx->enabled_list->count;
    if (!POR_WEAK || enabled <= 1 || s->score >= enabled) {
        Warning (debug, "Key already included");
        s->has_key = 1;
        return;
    }

    while ( !s->has_key ) {
        size_t min_score = INT32_MAX;
        int min_group = -1;
        ctx->nds_list[0]->count = 0;
        for (int i = 0; i < ctx->enabled_list->count; i++) {
            int group = ctx->enabled_list->data[i];
            if ( !(s->emit_status[group] & ES_SELECTED) ) continue;

            // check open NDSs
            size_t score = 0;
            ctx->nds_list[1]->count = 0;
            // First: all nds for the group's guards
            for (int t = 0; t < ctx->not_accords[group]->count && score < min_score; t++) {
                int tt = ctx->not_accords[group]->data[t];
                int fresh = (s->emit_status[tt] & ES_SELECTED) == 0;
                score += fresh ? ctx->group_score[tt] : 0;
                ctx->nds_list[1]->data[ctx->nds_list[1]->count] = tt;
                ctx->nds_list[1]->count += fresh;
            }

            if (score == 0) {
                Warning (debug, "Key is %d (all NDSs included)", group);
                return; // OK (all NDS's are in the SS)
            }

            if (score < min_score) {
                min_score = score;
                min_group = group;
                swap (ctx->nds_list[0], ctx->nds_list[1]);
            }
        }

        // add all the NDS's for the transition for which it is cheapest
        HREassert (ctx->nds_list[0]->count != 0);
        Printf (debug, "Adding NDSs: ");
        for (int t = 0; t < ctx->nds_list[0]->count; t++) {
            int tt = ctx->nds_list[0]->data[t];
            select_group (ctx, tt);
        }
        s->has_key = is_visible(ctx, min_group) ? -1 : 1;
        Warning (debug, "\nKey is %d (forced inclusion of NDSs)", min_group);

        beam_search (ctx); // may select a different search context!
        s = &ctx->search[ctx->search_order[0]];
    }
}

/**
 * For each state, this function sets up the current guard values etc
 * This setup is then reused by the analysis function
 */
static void
beam_setup (model_t model, por_context* ctx, int* src)
{
    por_init_transitions (model, ctx, src);

    por_transition_costs (ctx);

    // select an enabled transition group
    int c = RANDOM ? clock() : 0;
    ctx->beam_used = NO_BEAM ? 1 : ctx->enabled_list->count;
    for (int i = 0; i < ctx->beam_used; i++) {
        int enabled = i;
        if (RANDOM) enabled = (enabled + c) % ctx->beam_used;
        int group = ctx->enabled_list->data[enabled];
        // add to beam search
        ctx->search[i].work[0] = group;
        ctx->search[i].has_key = 0;
        // init work_enabled/work_disabled
        ctx->search[i].work_enabled = 1;
        ctx->search[i].work_disabled = ctx->ngroups;
        // init score
        ctx->search[i].score = 1;
        ctx->search[i].initialized = 0;
        memset(ctx->search[i].emit_status, 0, sizeof(char[ctx->ngroups]));

        int visible = is_visible (ctx, i);
        // reset counts
        ctx->search[i].visibles_selected = visible;
        ctx->search[i].enabled_selected = 1;
        ctx->search[i].ve_selected = visible;
        ctx->search[i].idx = i;

        ctx->search_order[i] = i;
    }
}

static int
por_beam_search_all (model_t self, int *src, TransitionCB cb, void *user_context)
{
    por_context* ctx = ((por_context*)GBgetContext(self));
    beam_setup (self, ctx, src);
    beam_search (ctx);
    beam_ensure_key (ctx);
    return beam_emit (ctx, src, cb, user_context);
}

/**
 * SCC Algorithm
 */

typedef struct scc_state_s {
    int              group;
    int              lowest;
} scc_state_t;

typedef struct scc_context_s {
    search_context  *search;            // context for each SCC search
    int              index;             // depth index
    int             *group_index;
    dfs_stack_t      tarjan;
    dfs_stack_t      stack;
    ci_list         *scc_list;
    ci_list        **stubborn_list;
    ci_list         *bad_scc;
    int              current_scc;
} scc_context_t;

static int SCC_SCC = -1;
static const int SCC_NEW = 0;

typedef enum {
    SCC_NO,
    SCC_OLD,
    SCC_BAD,
    SCC_CUR
} scc_type_e;

static inline scc_type_e
scc_is_scc (scc_context_t *scc, int group)
{
    int index = scc->group_index[group];
    if (index >= 0)
        return SCC_NO;
    if (scc->current_scc < index && index < 0) { // old SCC
        for (int i = 0; i < scc->bad_scc->count; i++) {
            if (index == scc->bad_scc->data[i]) return SCC_BAD;
        }
        return SCC_OLD;
    }
    return SCC_CUR;
}

static inline int
scc_expand (por_context *ctx, int group)
{
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;
    ci_list *successors;
    if (ctx->group_status[group] & GS_DISABLED) {
        int ns = find_cheapest_ns (ctx, scc->search, group);
        successors = ctx->ns[ns];
    } else if (POR_WEAK) {
        successors = ctx->not_left_accords[group];
    } else {
        successors = ctx->not_accords[group];
    }
    int count = 0;
    for (int j = 0; j < successors->count; j++) {
        int next_group = successors->data[j];
        switch (scc_is_scc(scc, next_group)) {
        case SCC_BAD: return -1;
        case SCC_NO: {
            scc_state_t next = { next_group, -1 };
            dfs_stack_push (scc->stack, (int*)&next);
            count++;
        }
        default: break;
        }
    }
    return count;
}

static inline void
reset_ns_scores (por_context* ctx, search_context *s, int group)
{
    if (NO_HEUR) return;

    // change the heuristic function according to selected group
    for (int k = 0 ; k < ctx->group2ns[group]->count; k++) {
        int ns = ctx->group2ns[group]->data[k];
        s->nes_score[ns] += ctx->group_score[group] << 1; // Make extra expensive
    }
}

static inline int
scc_ensure_key (por_context* ctx, int root)
{
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;
    int lowest = scc->group_index[root];

    // collect accepting states
    scc->stubborn_list[1]->count = 0;
    scc_state_t *x;
    for (int i = scc->scc_list->count - 1; i >= 0; i--) {
        int index = scc->scc_list->data[i];
        x = (scc_state_t *)dfs_stack_index (scc->tarjan, index);
        if (x->lowest < lowest)
            break;
        HREassert (!(ctx->group_status[x->group] & GS_DISABLED));
        scc->stubborn_list[1]->data[ scc->stubborn_list[1]->count++ ] = x->group;
    }

    Warning (debug, "Found %d enabled transitions in SCC",scc->stubborn_list[1]->count);

    // if no enabled transitions, return directly
    if (!POR_WEAK || scc->stubborn_list[1]->count == 0 ||
            scc->stubborn_list[1]->count == ctx->enabled_list->count) {
        return 0;
    }

    // mark SCC as lowest
    for (int i = 0; ; i++) {
        x = (scc_state_t *)dfs_stack_peek(scc->tarjan, i);
        scc->group_index[x->group] = lowest; // mark as current SCC
        if (x->group == root) break;
    }

    // Check
    for (int j=0; j < scc->stubborn_list[1]->count; j++) {
        bool allin = true;

        int group = scc->stubborn_list[1]->data[j];
        for (int g = 0; g < ctx->group2guard[group]->count && allin; g++) {
            int nds = ctx->group2guard[group]->data[g] + ctx->nguards;
            for (int k = 0; k < ctx->ns[nds]->count && allin; k++) {
                int ndsgroup = ctx->ns[nds]->data[k];
                allin &= scc->group_index[ndsgroup] == lowest;
            }
        }
        if (allin) {
            Warning (debug, "Found key");
            return 0; // all ok!
        }
    }

    // strongly expand one enabled state:
    int count = 0;
    int group = scc->stubborn_list[1]->data[0];
    for (int j = 0; j < ctx->not_accords[group]->count; j++) {
        int next_group = ctx->not_accords[group]->data[j];
        switch (scc_is_scc(scc, next_group)) {
        case SCC_BAD:
            // mark SCC as SCC
            for (int i = 0; ; i++) {
                x = (scc_state_t *)dfs_stack_peek(scc->tarjan, i);
                scc->group_index[x->group] = SCC_SCC; // mark as current SCC
                if (x->group == root) break;
            }
            SCC_SCC--;
            return -1;
        case SCC_NO:
            if (scc->group_index[next_group] >= lowest) break; // skip
            scc_state_t next = { next_group, -1 };
            dfs_stack_push (scc->stack, (int*)&next);
            count++;
        default: break;
        }
    }
    return count;
}

static inline bool
scc_root (por_context* ctx, int root)
{
    scc_state_t *x;
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;

    int count = 0;
    do {x = (scc_state_t *)dfs_stack_pop(scc->tarjan);
        reset_ns_scores (ctx, scc->search, x->group);
        scc->group_index[x->group] = SCC_SCC; // mark as current SCC
        count++;
    } while (x->group != root);

    // stubborn list was set by scc_ensure_key
    bool found_enabled = scc->stubborn_list[1]->count > 0;
    if (found_enabled) {
        //remove accepting
        scc->scc_list->count -= scc->stubborn_list[1]->count;

        Warning (debug, "Found stubborn SCC of size %d,%d!", scc->stubborn_list[1]->count, count);
        int count = scc->stubborn_list[0]->count;
        if (scc->stubborn_list[1]->count < count) {
            swap (scc->stubborn_list[0], scc->stubborn_list[1]);
            scc->bad_scc->data[scc->bad_scc->count++] = SCC_SCC; // remember stubborn SCC
            Warning (debug, "Update stubborn set %d --> %d!", count < INT32_MAX ? count : -1, scc->stubborn_list[0]->count);
        }
    }
    SCC_SCC--; // update to next SCC layer
    return found_enabled;
}

static void
scc_search (por_context* ctx)
{
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;
    scc_state_t *state, *pred;
    scc->scc_list->count = 0;

    while (scc->stubborn_list[0]->count != ctx->enabled_list->count) {
        state = (scc_state_t *)dfs_stack_top(scc->stack);
        if (state != NULL) {
            switch (scc_is_scc(scc, state->group)) {
            case SCC_BAD: return;
            case SCC_OLD:
            case SCC_CUR:
                dfs_stack_pop (scc->stack);
                continue;
            case SCC_NO: break;
            }

            if (scc->group_index[state->group] == SCC_NEW) {
                HREassert (state->lowest == -1);
                // assign index
                scc->group_index[state->group] = ++scc->index;
                state->lowest = scc->index;
                // add to tarjan stack
                if (!(ctx->group_status[state->group] & GS_DISABLED))
                    scc->scc_list->data[ scc->scc_list->count++ ] = dfs_stack_size(scc->tarjan);
                dfs_stack_push (scc->tarjan, &state->group);

                // push successors
                dfs_stack_enter (scc->stack);
                int count = scc_expand(ctx, state->group);
                if (count == -1) break; // bad SCC encountered
            } else if (dfs_stack_nframes(scc->stack) > 0){
                pred = (scc_state_t *)dfs_stack_peek_top (scc->stack, 1);
                if (scc->group_index[state->group] < pred->lowest) {
                    pred->lowest = scc->group_index[state->group];
                }
                dfs_stack_pop (scc->stack);
            }
        } else {
            state = (scc_state_t *)dfs_stack_peek_top (scc->stack, 1);
            if (scc->group_index[state->group] == state->lowest) {
                int to_explore = scc_ensure_key(ctx, state->group);
                if (to_explore == -1) { // bad state
                    dfs_stack_leave (scc->stack);
                    dfs_stack_pop (scc->stack);
                    Warning (debug, "Failed adding key");
                    break;
                } else if (to_explore > 0) {
                    Warning (debug, "Continuing search");
                    continue;
                } else {
                    Warning (debug, "Key added nothing or not needed");
                    dfs_stack_leave (scc->stack);
                }
            }

            dfs_stack_leave (scc->stack);
            state = (scc_state_t *)dfs_stack_top (scc->stack);

            update_ns_scores (ctx, scc->search, state->group); // remove from NS scores

            // detected an SCC
            if (scc->group_index[state->group] == state->lowest) {
                bool found = scc_root (ctx, state->group);
                if (found) break;
            } else if (dfs_stack_nframes(scc->stack) > 0) {
                // (after recursive return call) update index
                pred = (scc_state_t *)dfs_stack_peek_top (scc->stack, 1);
                if (state->lowest < pred->lowest) {
                    pred->lowest = state->lowest;
                }
            }
        }
    }
    HREassert (scc->stubborn_list[0]->count > 0 &&
               scc->stubborn_list[0]->count != INT32_MAX);
}

static void
empty_stack (scc_context_t *scc, dfs_stack_t stack)
{
    while (dfs_stack_size(stack) != 0) {
        scc_state_t *s = (scc_state_t *)dfs_stack_pop (stack);
        if (s == NULL) {
            dfs_stack_leave (stack);
        } else {
            scc->group_index[s->group] = SCC_NEW;
        }
    }
}

static void
scc_analyze (por_context* ctx)
{
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;
    scc->stubborn_list[0]->count = INT32_MAX;

    Warning (debug, "%s", "");
    scc->bad_scc->count = 0; // not SCC yet
    SCC_SCC = -1;
    for (int j=0; j < ctx->enabled_list->count; j++) {
        int group = ctx->enabled_list->data[j];
        if (scc->group_index[group] == SCC_NEW) {
            Warning (debug, "SCC search from %d", group);
            scc->index = 0;
            // SCC from current search will be leq the current value of SCC_SCC
            scc->current_scc = SCC_SCC;
            scc_state_t next = { group, -1 };
            dfs_stack_push (scc->stack, (int*)&next);
            scc_search (ctx);
            empty_stack (scc, scc->stack);  // clear stacks (and indices)
            empty_stack (scc, scc->tarjan); //    for next run
            int count = scc->stubborn_list[0]->count;
            if (alg == POR_SCC1 || count == 1 || count == ctx->enabled_list->count) {
                break; // early exit (one iteration or can't do better)
            }
        }
    }
}

static int
scc_emit (por_context* ctx, int* src, TransitionCB cb, void* uctx)
{
    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;

    if (ctx->enabled_list->count == scc->stubborn_list[0]->count) {
        // return all enabled
        proviso_hook_context_t provctx = {cb, uctx, 0, 0, 1};
        return GBgetTransitionsAll(ctx->parent, src, hook_cb, &provctx);
    } else {
        proviso_hook_context_t provctx = {cb, uctx, 0, 0, 0};
        provctx.force_proviso_true = !NO_L12 && check_L1_L2_proviso (ctx);

        int c = 0;
        for (int z = 0; z < scc->stubborn_list[0]->count; z++) {
            int i = scc->stubborn_list[0]->data[z];
            c += GBgetTransitionsLong (ctx->parent, i, src, hook_cb, &provctx);
        }
        return c;
    }
}

/**
 * For each state, this function sets up the current guard values etc
 * This setup is then reused by the analysis function
 */
static void
scc_setup (model_t model, por_context* ctx, int* src)
{
    por_init_transitions (model, ctx, src);

    por_transition_costs (ctx);

    scc_context_t *scc = (scc_context_t *)ctx->scc_ctx;
    memset(scc->search->emit_status, 0, sizeof(emit_status_t[ctx->ngroups]));
    memcpy(scc->search->nes_score, ctx->nes_score, NS_SIZE(ctx) * sizeof(int));
    memset(scc->group_index, SCC_NEW, ctx->ngroups * sizeof(int));
}

static int
por_scc_all (model_t self, int *src, TransitionCB cb, void *user_context)
{
    por_context* ctx = ((por_context*)GBgetContext(self));
    scc_setup (self, ctx, src);
    if (ctx->enabled_list->count == 0) return 0;
    scc_analyze (ctx);
    int emitted = scc_emit (ctx, src, cb, user_context);
    return emitted;
}

/**
 * DELETION algorithm.
 */

/**
 * Sets in deletion algorithm
 * Some are maintained only as stack or as set with cardinality counter
 * A "stack set" can both be iterated over and performed inclusion tests on,
 * however it does not support element removal (as it messes up the stack).
 */
typedef enum {
    DEL_N,  // set
    DEL_K,  // count set (stack content may be corrupted)
    DEL_E,  // set (EMITTED)
    DEL_Z,  // stack set
    DEL_R,  // set
    DEL_VD,
    DEL_VE,
    DEL_COUNT
} del_t;

typedef struct del_ctx_s {
    bms_t              *del;
    ci_list            *Kprime;
    ci_list            *Nprime;
    ci_list            *Dprime;
    int                 invisible_enabled;
    bool                has_invisible;
    bool                del_nes;
    bool                del_nds;
} del_ctx_t;

static del_ctx_t *
deletion_create (por_context* ctx)
{
    del_ctx_t *delctx = RTmalloc (sizeof(del_ctx_t));
    delctx->del = bms_create (ctx->ngroups, DEL_COUNT);
    delctx->Kprime = ci_create(ctx->ngroups);
    delctx->Nprime = ci_create(ctx->ngroups);
    delctx->Dprime = ci_create(ctx->ngroups);
    return delctx;
}

static inline bool
del_enabled (por_context* ctx, int u)
{
    return ctx->group_status[u] == GS_ENABLED;
}

static void
deletion_setup (model_t model, por_context* ctx, int* src, bool reset)
{
    del_ctx_t       *delctx = (del_ctx_t *) ctx->del_ctx;
    bms_t           *del = delctx->del;
    por_init_transitions (model, ctx, src);
    if (ctx->enabled_list->count == 0) return;

    // use -1 for deactivated NESs
    for (int ns = 0; ns < ctx->nguards; ns++) { // guard should be false
        ctx->nes_score[ns] = 0 - (ctx->label_status[ns] != 0); // 0 for false!
    }
    for (int ns = ctx->nguards; ns < NS_SIZE(ctx); ns++) { // guard should be true
        ctx->nes_score[ns] = 0 - (ctx->label_status[ns - ctx->nguards] == 0); // 0 for true!
    }

    // initially all active ns's are in:
    for (int t = 0; t < ctx->ngroups; t++) {
        ctx->group_score[t] = 0;
        if (del_enabled(ctx,t)) continue;
        for (int i = 0; i < ctx->group_has[t]->count; i++) {
            int ns = ctx->group_has[t]->data[i];
            ctx->group_score[t] += ctx->nes_score[ns] >= 0;
        }
    }

    // K = {}; Z := {}; KP := {}; TP := {}; R := {}
    // N := T
    bms_clear_lists (del);
    if (reset) {
        bms_set_all (del, DEL_N);
    } else {
        bms_and_or_all (del, DEL_R, DEL_E, DEL_N); // save emitted and revert
    }
    ci_clear (delctx->Kprime);
    ci_clear (delctx->Nprime);
    ci_clear (delctx->Dprime);

    //  K := A
    delctx->invisible_enabled = 0;
    for (int i = 0; i < ctx->enabled_list->count; i++) {
        int group = ctx->enabled_list->data[i];
        bms_push_new (del, DEL_K, group);

        bool d = bms_has(ctx->visible, NO_DYN_VIS ? VISIBLE : VISIBLE_NDS, group);
        bms_push_if (del, DEL_VD, group, d);
        bool e = NO_DYN_VIS && bms_has(ctx->visible, VISIBLE_NES, group);
        bms_push_if (del, DEL_VE, group, e);
        delctx->invisible_enabled += !is_visible(ctx, group);
    }
    delctx->has_invisible = delctx->invisible_enabled != 0;
    Warning (debug, "Deletion init |en| = %d \t|R| = %d",
             ctx->enabled_list->count, bms_count(del, DEL_R));
}

static inline bool
deletion_delete (por_context* ctx, bool *del_nes, bool *del_nds)
{
    del_ctx_t       *delctx = (del_ctx_t *) ctx->del_ctx;
    bms_t           *del = delctx->del;

    // search the deletion space:
    while (bms_count(del, DEL_Z) != 0 && bms_count(del, DEL_K) != 0) {
        int z = bms_pop (del, DEL_Z);
        Warning (debug, "Checking z = %d", z);

        if (bms_has(del,DEL_R,z)) return true;

        // First, the enabled transitions x that are still stubborn and
        // belong to DNA_u, need to be removed from key stubborn and put to Z.
        for (int i = 0; i < ctx->not_accords[z]->count && bms_count(del, DEL_K) > 0; i++) {
            int x = ctx->not_accords[z]->data[i];
            if (bms_has(del,DEL_K,x)) {
                if (!bms_has(del,DEL_N,x)) {
                    if (bms_has(del,DEL_R,x)) return true;
                    bms_push_new (del, DEL_Z, x);
                }
                if (bms_rem(del, DEL_K, x)) ci_add (delctx->Kprime, x);
                delctx->invisible_enabled -= !is_visible(ctx, x);
                if (delctx->has_invisible && delctx->invisible_enabled == 0 && !NO_V) {
                    return true;
                }
            }
        }
        if (bms_count(del, DEL_K) == 0) return true;

        // Second, the enabled transitions x that are still stubborn and
        // belong to DNB_u need to be removed from other stubborn and put to Z.
        for (int i = 0; i < ctx->not_left_accordsn[z]->count; i++) {
            int x = ctx->not_left_accordsn[z]->data[i];
            if (del_enabled(ctx,x) && bms_has(del,DEL_N,x)) {
                if (!bms_has(del,DEL_K,x)) {
                    if (bms_has(del,DEL_R,x)) return true;
                    bms_push_new (del, DEL_Z, x);
                }
                if (bms_rem(del, DEL_N, x)) ci_add (delctx->Nprime, x);
            }
        }

        ci_add (delctx->Dprime, z);
        // Third, the disabled transitions x, whose NES was stubborn
        // before removal of u, need to be put to Z.
        for (int i = 0; i < ctx->group2ns[z]->count; i++) {
            int ns = ctx->group2ns[z]->data[i];
            if (ctx->nes_score[ns] == -1) continue; // -1 is inactive!

            // not the first transition removed from NES?
            int score = ctx->nes_score[ns]++;
            if (score != 0) continue;

            for (int i = 0; i < ctx->group_hasn[ns]->count; i++) {
                int x = ctx->group_hasn[ns]->data[i];
                if (!del_enabled(ctx,x)) {
                    ctx->group_score[x]--;
                    HREassert (ctx->group_score[x] >= 0, "Wrong counting!");
                    if (ctx->group_score[x] == 0 && bms_has(del,DEL_N,x)) {
                        bms_push_new (del, DEL_Z, x);
                        HREassert (!bms_has(del, DEL_K, x)); // x is disabled
                        if (bms_rem(del, DEL_N, x)) ci_add (delctx->Nprime, x);
                    }
                }
            }
        }

        // Fourth, if a visible is deleted, then remove all enabled visible
        if ((SAFETY || PINS_LTL) && !*del_nds && !*del_nes &&
                (NO_V ? del_enabled(ctx,z) : is_visible(ctx,z))) {
            if (NO_DYN_VIS || bms_has(ctx->visible, VISIBLE_NES, z)) {
                for (int i = 0; i < del->lists[DEL_VD]->count; i++) {
                    int x = del->lists[DEL_VD]->data[i];
                    if (bms_has(del, DEL_N, x) || bms_has(del, DEL_K, x)) {
                        if (bms_rem(del, DEL_N, x)) ci_add (delctx->Nprime, x);
                        if (bms_rem(del, DEL_K, x)) {
                            ci_add (delctx->Kprime, x);
                            if (bms_count(del, DEL_K) == 0) return true;
                            delctx->invisible_enabled -= !is_visible(ctx, x);
                            if (delctx->has_invisible && delctx->invisible_enabled == 0) return true;
                        }
                        if (bms_has(del,DEL_R,x)) return true; // enabled
                        bms_push_new (del, DEL_Z, x);
                    }
                }
                *del_nds  = true;
                *del_nes |= NO_DYN_VIS;
            }
            if (!NO_DYN_VIS && bms_has(ctx->visible, VISIBLE_NDS, z)) {
                for (int i = 0; i < del->lists[DEL_VE]->count; i++) {
                    int x = del->lists[DEL_VE]->data[i];
                    if (bms_has(del, DEL_N, x) || bms_has(del, DEL_K, x)) {
                        if (bms_rem(del, DEL_N, x)) ci_add (delctx->Nprime, x);
                        if (bms_rem(del, DEL_K, x)) {
                            ci_add (delctx->Kprime, x);
                            if (bms_count(del, DEL_K) == 0) return true;
                            delctx->invisible_enabled -= !is_visible(ctx, x);
                            if (delctx->has_invisible && delctx->invisible_enabled == 0) return true;
                        }
                        if (bms_has(del,DEL_R,x)) return true; // enabled
                        bms_push_new (del, DEL_Z, x);
                    }
                }
                *del_nes = true;
            }
        }
    }
    return bms_count(del, DEL_K) == 0;
}

static inline void
deletion_analyze (por_context *ctx)
{
    if (ctx->enabled_list->count == 0) return;
    del_ctx_t          *delctx = (del_ctx_t *) ctx->del_ctx;
    bms_t              *del = delctx->del;
    bool                del_nes = false;
    bool                del_nds = false;

    for (int i = 0; i < ctx->enabled_list->count && bms_count(del, DEL_K) > 1; i++) {
        int v = ctx->enabled_list->data[i];
        if (bms_has(del, DEL_R, v)) continue;

        if (bms_rem(del, DEL_K, v)) ci_add (delctx->Kprime, v);
        if (bms_rem(del, DEL_N, v)) ci_add (delctx->Nprime, v);
        bms_push_new (del, DEL_Z, v);

        Warning (debug, "Deletion start from v = %d: |E| = %d \t|K| = %d", v, ctx->enabled_list->count, bms_count(del, DEL_K));

        bool            del_nes_old = del_nes;
        bool            del_nds_old = del_nds;
        bool revert = deletion_delete (ctx, &del_nes, &del_nds);

        while (bms_count(del, DEL_Z) != 0) bms_pop (del, DEL_Z);

        // Reverting deletions if necessary
        if (revert) {
            Warning (debug, "Deletion rollback: |T'| = %d \t|K'| = %d \t|D'| = %d",
                     ci_count(delctx->Nprime), ci_count(delctx->Kprime), ci_count(delctx->Dprime));
            bms_add (del, DEL_R, v); // fail transition!
            while (ci_count(delctx->Kprime) != 0) {
                int x = ci_pop (delctx->Kprime);
                bool seen = bms_push_new (del, DEL_K, x);
                delctx->invisible_enabled += !is_visible(ctx, x);
                HREassert (seen, "DEL_K messed up");
            }
            while (ci_count(delctx->Nprime) != 0) {
                int x = ci_pop (delctx->Nprime);
                del->set[x] = del->set[x] | (1<<DEL_N);
            }
            while (ci_count(delctx->Dprime) != 0) {
                int x = ci_pop (delctx->Dprime);
                for (int i = 0; i < ctx->group2ns[x]->count; i++) {
                    int ns = ctx->group2ns[x]->data[i];
                    ctx->nes_score[ns] -= (ctx->nes_score[ns] >= 0);
                    if (ctx->nes_score[ns] != 0) continue; // NES not readded

                    for (int i = 0; i < ctx->group_hasn[ns]->count; i++) {
                        int x = ctx->group_hasn[ns]->data[i];
                        ctx->group_score[x] += !del_enabled(ctx,x);
                    }
                }
            }
            del_nes &= del_nes_old; // remain only true if successfully removed before
            del_nds &= del_nds_old; // remain only true if successfully removed before
        } else {
            ci_clear (delctx->Kprime);
            ci_clear (delctx->Nprime);
            ci_clear (delctx->Dprime);
        }
    }

    delctx->del_nes = del_nes;
    delctx->del_nds = del_nds;
}

static inline int
deletion_emit_new (por_context *ctx, proviso_hook_context_t *provctx, int* src)
{
    del_ctx_t       *delctx = (del_ctx_t *) ctx->del_ctx;
    bms_t           *del = delctx->del;
    int c = 0;
    for (int z = 0; z < ctx->enabled_list->count; z++) {
        int i = ctx->enabled_list->data[z];
        if (por_is_stubborn(ctx,i) && !bms_has(del,DEL_E,i)) {
            del->set[i] |= 1<<DEL_E | 1<<DEL_R;
            c += GBgetTransitionsLong (ctx->parent, i, src, hook_cb, provctx);
        }
    }
    return c;
}

static inline int
deletion_emit (model_t model, por_context *ctx, int *src, TransitionCB cb,
               void *uctx)
{
    del_ctx_t          *delctx = (del_ctx_t *) ctx->del_ctx;
    bms_t              *del = delctx->del;
    proviso_hook_context_t provctx = {cb, uctx, 0, 0, 0};

    if (PINS_LTL || SAFETY) {
        if (NO_L12) {
            int selected = 0;
            for (int i = 0; i < ctx->enabled_list->count; i++) {
                int x = ctx->enabled_list->data[i];
                selected += por_is_stubborn(ctx,x);
            }
            provctx.force_proviso_true = (selected == ctx->enabled_list->count);
        } else { // Deletion guarantees that I holds, but does V hold?
            provctx.force_proviso_true = !delctx->del_nds && !delctx->del_nes;
        }
    }

    int emitted = deletion_emit_new (ctx, &provctx, src);

    // emit more if we need to fulfill a liveness / safety proviso
    if ( ( PINS_LTL && provctx.por_proviso_false_cnt != 0) ||
         (!PINS_LTL && provctx.por_proviso_true_cnt  == 0) ) {
        if (NO_L12) {
            for (int i = 0; i < ctx->enabled_list->count; i++) {
                int x = ctx->enabled_list->data[i];
                del->set[x] |= (1 << DEL_N);
            }
            emitted += deletion_emit_new (ctx, &provctx, src);
        } else {
            for (int i = 0; i < del->lists[DEL_VD]->count; i++) {
                int x = del->lists[DEL_VD]->data[i];
                del->set[x] |= 1<<DEL_R;
            }
            for (int i = 0; i < del->lists[DEL_VE]->count && !NO_DYN_VIS; i++) {
                int x = del->lists[DEL_VE]->data[i];
                del->set[x] |= 1<<DEL_R;
            }
            deletion_setup (model, ctx, src, false);
            deletion_analyze (ctx);

            emitted += deletion_emit_new (ctx, &provctx, src);
        }
    }

    return emitted;
}

static int
por_deletion_all (model_t self, int *src, TransitionCB cb, void *user_context)
{
    por_context* ctx = ((por_context*)GBgetContext(self));
    deletion_setup (self, ctx, src, true);
    deletion_analyze (ctx);
    return deletion_emit (self, ctx, src, cb, user_context);
}

static void
list_invert (ci_list *list)
{
    for (int i = 0; i < list->count / 2; i++) {
        swap (list->data[i], list->data[list->count - i - 1]);
    }
}

static scc_context_t *
create_scc_ctx (por_context* ctx)
{
    scc_context_t *scc = RTmallocZero (sizeof(scc_context_t));
    scc->group_index = RTmallocZero ((ctx->ngroups) * sizeof(int));
    scc->scc_list = RTmallocZero ((ctx->ngroups + 1) * sizeof(int));
    scc->stubborn_list = RTmallocZero (sizeof(ci_list *[2]));
    scc->stubborn_list[0] = ci_create (ctx->ngroups);
    scc->stubborn_list[1] = ci_create (ctx->ngroups);
    scc->bad_scc = ci_create (ctx->ngroups);
    scc->stack = dfs_stack_create (2); // only integers for groups
    scc->tarjan = dfs_stack_create (1); // only integers for group
    scc->search = &ctx->search[0];
    ctx->search_order[0] = 0;
    return scc;
}

/**
 * Function used to setup the beam search
 * Initialize and allocate all memory that is reused all the time
 * Setup pointers to functions used in beam search
 *
 * Additionally, it combines gaurd info with MCE for quick NES/NDS access.
 */
static search_context *
create_beam_context (por_context *ctx)
{
    // setup a fixed maximum of number of search contexts
    // since we use at most one search context per transition group, use n
    const int BEAM_WIDTH = ctx->ngroups;

    search_context *search = RTmallocZero(sizeof(search_context[BEAM_WIDTH]));
    ctx->group_status = RTmallocZero(ctx->ngroups * sizeof(char));
    ctx->group_score  = RTmallocZero(ctx->ngroups * sizeof(int));
    ctx->label_status = RTmallocZero(ctx->nguards * sizeof(int));
    ctx->search_order = RTmallocZero(BEAM_WIDTH * sizeof(int));
    ctx->nes_score    = RTmallocZero(NS_SIZE(ctx) * sizeof(int));
    ctx->beam_width = BEAM_WIDTH;
    ctx->beam_used = BEAM_WIDTH;
    ctx->emitted = 0;

    for (int i = 0 ; i < BEAM_WIDTH; i++) {
        ctx->search_order[i] = i;
        search[i].emit_status = RTmallocZero(ctx->ngroups * sizeof(emit_status_t));
        search[i].work = RTmallocZero(ctx->ngroups * sizeof(int));
        search[i].work_enabled = 0;
        search[i].work_disabled = ctx->ngroups;
        search[i].score = 0;
        search[i].nes_score = RTmallocZero(sizeof(int[NS_SIZE(ctx)]));
    }
    return search;
}

/**
 * Default functions for long and short
 * Note: these functions don't work for partial order reduction,
 *       because partial order reduction selects a subset of the transition
 *       group and doesn't know beforehand whether to emit this group or not
 */
static int
por_long (model_t self, int group, int *src, TransitionCB cb, void *ctx)
{
    (void)self; (void)group; (void)src; (void)cb; (void)ctx;
    Abort ("Using Partial Order Reduction in combination with --grey or -reach? Long call failed.");
}

static int
por_short (model_t self, int group, int *src, TransitionCB cb, void *ctx)
{
    (void)self; (void)group; (void)src; (void)cb; (void)ctx;
    Abort ("Using Partial Order Reduction in combination with -reach or --cached? Short call failed.");
}

static inline bool
guard_of (por_context *ctx, int i, matrix_t *m, int j)
{
    for (int g = 0; g < ctx->group2guard[i]->count; g++) {
        int guard = ctx->group2guard[i]->data[g];
        if (dm_is_set (m, guard, j)) {
            return true;
        }
    }
    return false;
}

/**
 * Setup the partial order reduction layer.
 * Reads available dependencies, i.e. read/writes dependencies when no
 * NDA/NES/NDS is provided.
 */
model_t
GBaddPOR (model_t model)
{
    if (GBgetAcceptingStateLabelIndex(model) != -1) {
        Print1  (info, "POR layer: model may be a buchi automaton.");
        Print1  (info, "POR layer: use LTSmin's own LTL layer (--ltl) for correct POR.");
    }

    // check support for guards, fail without
    if (!GBhasGuardsInfo(model)) {
        Print1 (info, "Frontend doesn't have guards. Ignoring --por.");
        return model;
    }
    if (NO_HEUR_BEAM) NO_HEUR = NO_BEAM = 1;
    if (!RANDOM && (NO_HEUR || NO_BEAM)) {
        if (__sync_bool_compare_and_swap(&RANDOM, 0, 1)) { // static variable
            Warning (info, "Using random selection instead of heuristics / multiple Beam searches.");
        }
    }

    // do the setup
    model_t             pormodel = GBcreateBase ();

    por_context *ctx = RTmalloc (sizeof *ctx);
    ctx->parent = model;

    sl_group_t *guardLabels = GBgetStateLabelGroupInfo (model, GB_SL_GUARDS);
    sl_group_t* sl_guards = GBgetStateLabelGroupInfo(model, GB_SL_ALL);
    ctx->nguards = guardLabels->count;
    ctx->nlabels = pins_get_state_label_count(model);
    ctx->ngroups = pins_get_group_count(model);
    ctx->nslots = pins_get_state_variable_count (model);
    HREassert (ctx->nguards <= ctx->nlabels);
    HREassert (guardLabels->sl_idx[0] == 0, "Expecting guards at index 0 of all labels.");

    Print1 (info, "Initializing POR dependencies: labels %d, guards %d",
            ctx->nlabels, ctx->nguards);

    matrix_t           *p_dm = NULL;
    matrix_t           *p_dm_w = GBgetDMInfoMayWrite (model);
    int id = GBgetMatrixID (model, LTSMIN_MATRIX_ACTIONS_READS);
    if (id == SI_INDEX_FAILED) {
        p_dm = GBgetDMInfo (model);
    } else {
        p_dm = GBgetMatrix (model, id);
    }
    HREassert (dm_ncols(p_dm) == ctx->nslots && dm_nrows(p_dm) == ctx->ngroups);

    // guard to group dependencies
    matrix_t        *p_sl = GBgetStateLabelInfo(model);
    matrix_t label_is_dependent;
    dm_create(&label_is_dependent, ctx->nlabels, ctx->ngroups);
    for (int i = 0; i < ctx->nlabels; i++) {
        for (int j = 0; j < ctx->ngroups; j++) {
            for (int k = 0; k < ctx->nslots; k++) {
                if (dm_is_set(p_sl, sl_guards->sl_idx[i], k) && dm_is_set(p_dm_w, j, k)) {
                    dm_set( &label_is_dependent, i, j );
                    break;
                }
            }
        }
    }

    // extract inverse relation, transition group to guard
    matrix_t gg_matrix;
    dm_create(&gg_matrix, ctx->ngroups, ctx->nguards);
    for (int i = 0; i < ctx->ngroups; i++) {
        guard_t *g = GBgetGuard(model, i);
        HREassert (g != NULL, "GUARD RETURNED NULL %d", i);
        for (int j = 0; j < g->count; j++) {
            dm_set(&gg_matrix, i, g->guard[j]);
        }
    }
    ctx->guard2group            = (ci_list **) dm_cols_to_idx_table(&gg_matrix);
    ctx->group2guard            = (ci_list **) dm_rows_to_idx_table(&gg_matrix);
    dm_free(&gg_matrix);

    // mark minimal necessary enabling set
    matrix_t *p_gnes_matrix = GBgetGuardNESInfo(model);
    NO_NES |= p_gnes_matrix == NULL;

    if (!NO_NES) {
        HREassert (dm_nrows(p_gnes_matrix) == ctx->nlabels &&
                   dm_ncols(p_gnes_matrix) == ctx->ngroups);
        // copy p_gnes_matrix to gnes_matrix, then optimize it
        dm_copy(p_gnes_matrix, &ctx->label_nes_matrix);
    } else {
        dm_create(&ctx->label_nes_matrix, ctx->nlabels, ctx->ngroups);
        for (int i = 0; i < ctx->nlabels; i++) {
            for (int j = 0; j < ctx->ngroups; j++) {
                dm_set (&ctx->label_nes_matrix, i, j);
            }
        }
    }
    // optimize nes
    // remove all transition groups that do not write to this guard
    for (int i = 0; i < ctx->nlabels; i++) {
        for (int j = 0; j < ctx->ngroups; j++) {
            // if guard i has group j in the nes, make sure
            // the group writes to the same part of the state
            // vector the guard reads from, otherwise
            // this value can be removed
            if (dm_is_set(&ctx->label_nes_matrix, i, j)) {
                if (!dm_is_set(&label_is_dependent, i, j)) {
                    dm_unset(&ctx->label_nes_matrix, i, j);
                }
            }
        }
    }
    ctx->label_nes   = (ci_list **) dm_rows_to_idx_table(&ctx->label_nes_matrix);

    // same for nds
    matrix_t *label_nds_matrix = GBgetGuardNDSInfo(model);
    NO_NDS |= label_nds_matrix == NULL;

    if (!NO_NDS) {
        HREassert (dm_nrows(label_nds_matrix) == ctx->nlabels &&
                   dm_ncols(label_nds_matrix) == ctx->ngroups);
        // copy p_gnds_matrix to gnes_matrix, then optimize it
        dm_copy(label_nds_matrix, &ctx->label_nds_matrix);
    } else {
        dm_create(&ctx->label_nds_matrix, ctx->nlabels, ctx->ngroups);
        for (int i = 0; i < ctx->nlabels; i++) {
            for (int j = 0; j < ctx->ngroups; j++) {
                dm_set (&ctx->label_nds_matrix, i, j);
            }
        }
    }

    // optimize nds matrix
    // remove all transition groups that do not write to this guard
    for (int i = 0; i < ctx->nlabels; i++) {
        for (int j = 0; j < ctx->ngroups; j++) {
            // if guard i has group j in the nes, make sure
            // the group writes to the same part of the state
            // vector the guard reads from, otherwise
            // this value can be removed
            if (dm_is_set(&ctx->label_nds_matrix, i, j)) {
                if (!dm_is_set(&label_is_dependent, i, j)) {
                    dm_unset(&ctx->label_nds_matrix, i, j);
                }
            }
        }
    }
    ctx->label_nds   = (ci_list **) dm_rows_to_idx_table(&ctx->label_nds_matrix);

    matrix_t nds;
    dm_create(&nds, ctx->ngroups, ctx->ngroups);
    for (int i = 0; i < ctx->ngroups; i++) {
        for (int j = 0; j < ctx->ngroups; j++) {
            if (guard_of(ctx, i, &ctx->label_nds_matrix, j)) {
                dm_set( &nds, i, j);
            }
        }
    }

    // extract guard not co-enabled and guard-nes information
    // from guard may-be-co-enabled with guard relation:
    // for a guard g, find all guards g' that may-not-be co-enabled with it
    // then, for each g', mark all groups in gnce_matrix
    matrix_t *label_mce_matrix = GBgetGuardCoEnabledInfo(model);
    NO_MC |= label_mce_matrix == NULL;
    if (NO_MC && !NO_MCNDS) {
        if (__sync_bool_compare_and_swap(&NO_MCNDS, 0, 1)) { // static variable
            Warning (info, "No maybe-coenabled matrix found. Turning off NESs from NDS+MC.");
        }
    }

    if (!NO_MC) {
        HREassert (dm_ncols(label_mce_matrix) >= ctx->nguards &&
                   dm_nrows(label_mce_matrix) >= ctx->nguards);
        dm_create(&ctx->gnce_matrix, ctx->nguards, ctx->ngroups);
        dm_create(&ctx->nce, ctx->ngroups, ctx->ngroups);
        for (int g = 0; g < ctx->nguards; g++) {
            // iterate over all guards
            for (int gg = 0; gg < ctx->nguards; gg++) {
                // find all guards that may not be co-enabled
                if (dm_is_set(label_mce_matrix, g, gg)) continue;

                // gg may not be co-enabled with g, find all
                // transition groups in which it is used
                for (int tt = 0; tt < ctx->guard2group[gg]->count; tt++) {
                    dm_set(&ctx->gnce_matrix, g, ctx->guard2group[gg]->data[tt]);

                    for (int t = 0; t < ctx->guard2group[g]->count; t++) {
                        dm_set(&ctx->nce, ctx->guard2group[g]->data[t],
                                            ctx->guard2group[gg]->data[tt]);
                    }
                }
            }
        }
        ctx->guard_nce             = (ci_list **) dm_rows_to_idx_table(&ctx->gnce_matrix);
    }

    // extract accords with matrix
    matrix_t *not_accords_with = GBgetDoNotAccordInfo(model);
    NO_DNA |= not_accords_with == NULL;

    HREassert (NO_DNA || (dm_nrows(not_accords_with) == ctx->ngroups &&
                          dm_ncols(not_accords_with) == ctx->ngroups));

    // Combine Do Not Accord with dependency and other information
    dm_create(&ctx->not_accords_with, ctx->ngroups, ctx->ngroups);
    for (int i = 0; i < ctx->ngroups; i++) {
        for (int j = 0; j < ctx->ngroups; j++) {
            if (i == j) {
                dm_set(&ctx->not_accords_with, i, j);
            } else {

                if ( !NO_MC && dm_is_set(&ctx->nce, i , j) ) {
                    continue; // transitions never coenabled!
                }

                if ( !NO_DNA && !dm_is_set(not_accords_with, i , j) ) {
                    continue; // transitions accord with each other
                }

                if (dm_is_set(&nds,i,j) || dm_is_set(&nds,j,i)) {
                    dm_set( &ctx->not_accords_with, i, j );
                    continue;
                }

                // is dependent?
                for (int k = 0; k < ctx->nslots; k++) {
                    if ((dm_is_set( p_dm_w, i, k) && dm_is_set( p_dm, j, k)) ||
                        (dm_is_set( p_dm, i, k) && dm_is_set( p_dm_w, j, k)) ) {
                        dm_set( &ctx->not_accords_with, i, j );
                        break;
                    }
                }
            }
        }
    }
    ctx->not_accords = (ci_list **) dm_rows_to_idx_table(&ctx->not_accords_with);

    matrix_t *commutes = GBgetCommutesInfo (model);
    NO_COMMUTES |= commutes == NULL;

    if (POR_WEAK && (NO_NES || NO_NDS)) {
        if (__sync_bool_compare_and_swap(&POR_WEAK, 1, 0)) { // static variable
            Warning (info, "No NES/NDS, which is required for weak relations. Switching to strong stubborn sets.");
        }
    }

    if (!POR_WEAK) {
        ctx->not_left_accords  = ctx->not_accords;
        ctx->not_left_accordsn = ctx->not_accords;
    } else {
        matrix_t *must_disable = NULL;
        int id = GBgetMatrixID(model, LTSMIN_MUST_DISABLE_MATRIX);
        if (id != SI_INDEX_FAILED) {
            must_disable = GBgetMatrix(model, id);
        } else {
            Print1 (info, "No must-disable matrix available for weak sets.");
            NO_MDS = 1;
        }

        matrix_t not_left_accords;
        dm_create(&not_left_accords, ctx->ngroups, ctx->ngroups);
        for (int i = 0; i < ctx->ngroups; i++) {
            for (int j = 0; j < ctx->ngroups; j++) {
                if (i == j) continue;

                if (!NO_MDS && guard_of(ctx, i, must_disable, j)) {
                    continue;
                }

                if (guard_of(ctx, i, &ctx->label_nes_matrix, j) || dm_is_set(&nds, j, i)) {
                    dm_set( &not_left_accords, i, j );
                    continue;
                }

                if (NO_COMMUTES) {
                    if (!dm_is_set(&ctx->not_accords_with, i, j)) continue;
                } else {
                    if ( dm_is_set(commutes, i , j) ) continue;
                }

                // is even dependent? Front-end might miss it.
                for (int k = 0; k < ctx->nslots; k++) {
                    if ((dm_is_set( p_dm_w, i, k) && dm_is_set( p_dm, j, k)) ||
                        (dm_is_set( p_dm, i, k) && dm_is_set( p_dm_w, j, k)) ) {
                        dm_set( &not_left_accords, i, j );
                        break;
                    }
                }
            }
        }
        ctx->not_left_accords = (ci_list **) dm_rows_to_idx_table(&not_left_accords);
        ctx->not_left_accordsn= (ci_list **) dm_cols_to_idx_table(&not_left_accords);
    }

    // free temporary matrices
    dm_free (&label_is_dependent);
    dm_free (&nds);

    // setup global group_in/group_has relation
    // idea, combine nes and nds in one data structure (ns, necessary set)
    // each guard is either disabled (use nes) or enabled (nds)
    // setup: ns = [0...n_guards-1] nes, [n_guards..2n_guards-1] nds
    // this way the search algorithm can skip the nes/nds that isn't needed based on
    // guard_status, using <n or >=n as conditions
    matrix_t group_in;
    dm_create (&group_in, NS_SIZE(ctx), ctx->ngroups);
    for (int i = 0; i < ctx->nguards; i++) {
        for (int j = 0; j < ctx->label_nes[i]->count; j++) {
            dm_set(&group_in, i, ctx->label_nes[i]->data[j]);
        }
        if (!NO_MCNDS) { // add NDS range:
            for (int j = 0; j < ctx->label_nds[i]->count; j++) {
                int group = ctx->label_nds[i]->data[j];
                dm_set(&group_in, i+ctx->nguards, group);
            }
        }
    }
    // build tables ns, and group in
    ctx->ns = (ci_list**) dm_rows_to_idx_table(&group_in);
    ctx->group2ns = (ci_list**) dm_cols_to_idx_table(&group_in);
    dm_free (&group_in);

    // group has relation
    // mapping [0...n_guards-1] disabled guard (nes)
    // mapping [n_guards..2*n_guards-1] enabled guard (nds)
    // group has relation is more difficult because nds
    // needs not co-enabled info and transition groups
    matrix_t group_has;
    dm_create (&group_has, ctx->ngroups, NS_SIZE(ctx));
    for (int i = 0; i < ctx->nguards; i++) {
        // nes
        for (int j = 0; j < ctx->guard2group[i]->count; j++) {
            dm_set(&group_has, ctx->guard2group[i]->data[j], i);
        }
        // nds
        if (!NO_MCNDS) { // add NDS range:
            for (int j = 0; j < ctx->guard_nce[i]->count; j++) {
                dm_set(&group_has, ctx->guard_nce[i]->data[j], i + ctx->nguards);
            }
        }
    }
    // build table group has
    ctx->group_has = (ci_list**) dm_rows_to_idx_table(&group_has);
    ctx->group_hasn = (ci_list**) dm_cols_to_idx_table(&group_has);
    dm_free (&group_has);

    if (PREFER_NDS) {
        for (int i = 0; i < ctx->ngroups; i++) {
            list_invert (ctx->group_has[i]);
        }
    }

    GBsetContext (pormodel, ctx);
    GBsetNextStateLong  (pormodel, por_long);
    GBsetNextStateShort (pormodel, por_short);
    switch (alg) {
    case POR_SCC:
    case POR_SCC1: GBsetNextStateAll   (pormodel, por_scc_all);  break;
    case POR_HEUR: GBsetNextStateAll   (pormodel, por_beam_search_all); break;
    case POR_DEL:  GBsetNextStateAll   (pormodel, por_deletion_all);     break;
    default: Abort ("Unknown POR algorithm: '%s'", key_search(por_algorithm, alg));
    }

    GBinitModelDefaults (&pormodel, model);

    if (GBgetPorGroupVisibility(pormodel) == NULL) {
        // reserve memory for group visibility, will be provided by ltl layer or tool
        ctx->group_visibility = RTmallocZero( ctx->ngroups * sizeof(int) );
        GBsetPorGroupVisibility  (pormodel, ctx->group_visibility);
    } else {
        ctx->group_visibility = GBgetPorGroupVisibility(pormodel);
    }
    if (GBgetPorStateLabelVisibility(pormodel) == NULL) {
        // reserve memory for group visibility, will be provided by ltl layer or tool
        ctx->label_visibility = RTmallocZero( ctx->nlabels * sizeof(int) );
        GBsetPorStateLabelVisibility  (pormodel, ctx->label_visibility);
    } else {
        ctx->label_visibility = GBgetPorStateLabelVisibility (pormodel);
    }

    ctx->enabled_list = ci_create (ctx->ngroups);
    ctx->nds_list[0] = ci_create (ctx->ngroups);
    ctx->nds_list[1] = ci_create (ctx->ngroups);

    int                 s0[ctx->nslots];
    GBgetInitialState (model, s0);
    GBsetInitialState (pormodel, s0);

    ctx->search  = create_beam_context (ctx);
    ctx->scc_ctx = create_scc_ctx (ctx);
    ctx->del_ctx = deletion_create (ctx);

    return pormodel;
}

bool
por_is_stubborn (por_context *ctx, int group)
{
    switch (alg) {
        case POR_SCC:
        case POR_SCC1: Abort ("Unimplemented SCC + check");
        case POR_HEUR: {
            search_context *s = &ctx->search[ctx->search_order[0]];
            return s->score >= ctx->enabled_list->count ||
                  (s->emit_status[group] & ES_SELECTED);
        }
        case POR_DEL: {
            del_ctx_t *del_ctx = (del_ctx_t *)ctx->del_ctx;
            bms_t* del = del_ctx->del;
            return bms_has(del, DEL_N, group) || bms_has(del, DEL_K, group);
        }
    default: Abort ("Unknown POR algorithm: '%s'", key_search(por_algorithm, alg));
    }
}
