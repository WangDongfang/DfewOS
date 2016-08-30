/*==============================================================================
** logo.c -- when shart gui, run this appliction.
**
** MODIFY HISTORY:
**
** 2011-11-03 wdf Create.
==============================================================================*/
#include <dfewos.h>
#include "../../string.h"
#include "../gui.h"

/*======================================================================
  logo strings
======================================================================*/
static char *_G_logo[] =
{
    "                                 ]]]]",
    "                               ]]]]",
    "                              ]]]",
    "                             ]]]",
    "                            ]]]",
    "                           ]]]",
    "                          ]]]",
    "]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]]",
    "                        ]]]",
    "            ]]]]]]     ]]]             ]]]",
    "          ]]]     ]]] ]]]]              ]]]",
    "        ]]]          ]]]]]      ]]]      ]]",
    "        ]]]         ]]] ]]     ]] ]]     ]]",
    "        ]]]        ]]]  ]]]   ]]  ]]    ]]",
    "          ]]]    ]]]     ]]] ]]    ]]  ]]",
    "            ]]]]]]        ]]]]      ]]]]         (TM)"
};

/*==============================================================================
 * - app_logo()
 *
 * - logo application entry. when gui init, first start this app
 */
void app_logo ()
{
    int ix;
    int logo_width = 55; /* max char num per line */
    int logo_lines = (int)N_ELEMENTS(_G_logo);
    GUI_SIZE scr_size;
    GUI_COOR logo_start;

    GUI_CBI *pCBI_logo = malloc (sizeof (GUI_CBI));

    if (pCBI_logo == NULL) {
        return;
    }

    /* calculate logo start coordinate */
    gra_get_scr_size (&scr_size);
    logo_start.x = (scr_size.w - logo_width * GUI_FONT_WIDTH) / 2; 
    logo_start.y = (scr_size.h - logo_lines * GUI_FONT_HEIGHT) / 2;

    strcpy (pCBI_logo->name, "logo cbi");

    pCBI_logo->left_up.x = 0;
    pCBI_logo->left_up.y = 0;
    pCBI_logo->right_down.x = scr_size.w - 1;
    pCBI_logo->right_down.y = scr_size.h - 1;

    pCBI_logo->func_press   = cbf_default_press;
    pCBI_logo->func_leave   = cbf_default_leave;
    pCBI_logo->func_release = cbf_go_home;
    pCBI_logo->func_drag    = cbf_hw_drag;

    pCBI_logo->data = NULL;

    cbi_register (pCBI_logo);

    /* print logo */
    for (ix = 0; ix < logo_lines; ix++) {
        font_draw_string (&logo_start, GUI_COLOR_RED, (uint8 *)_G_logo[ix], strlen(_G_logo[ix]));
        logo_start.y += GUI_FONT_HEIGHT;
    }

    /* test hanzi */
#if 0
    #define HAN_LOGO "我们是共产主义事业的接班人！"
    han_draw_string (&logo_start, GUI_COLOR_YELLOW, (uint8 *)HAN_LOGO, strlen(HAN_LOGO));
    han_draw_string_v (&logo_start, GUI_COLOR_YELLOW, (uint8 *)HAN_LOGO, strlen(HAN_LOGO));
#endif
}

/*==============================================================================
** FILE END
==============================================================================*/
