#include "name.h"
#include <ctype.h>
#include <tchar.h>
#include <windows.h>

bool catalogue_update(const TCHAR *target_path, const TCHAR *base_path, size_t level)
{
    
}

struct level_context {
    enum level_state {
        STATE_NORMAL = 0,
        STATE_WORD0,
        STATE_PAR0,
        STATE_SC0,
        STATE_DA0,

        STATE_WH1,
        STATE_WORD0,
        STATE_WORD1,
        STATE_WH2,
        STATE_WH3,
        STATE_PAR1,
        STATE_WH4,
        STATE_UP0,
        STATE_UP1,
        OFFSET_COMMENT,
    } state;
    size_t pos;
};

const TCHAR *get_level(const TCHAR *str, size_t len, size_t level, struct level_context *p_context, size_t *p_len)
{
    enum level_state state = p_context->state;
    size_t pos = p_context->pos, begin = 0, end = 0, comment = 0,  = 0;

    for (TCHAR ch = str[pos]; ch; ch = str[++pos])
    {
        switch (state)
        {
        case STATE_NORMAL:
            switch (ch)
            {
            case _T('('):

            }


            if (_istspace(ch)) upd = 1;
            else state++;
            break;
        case STATE_WORD0:
            end = begin = pos;
            state++;
            break;
        case STATE_PAR0:
            if (ch == _T('(')) upd = 1, state += OFFSET_COMMENT;
            else state++;
            break;
        case STATE_SC0:
            if (ch == _T(';')) upd = 1, 


        case STATE_WORD0:
            end = begin = pos;
            state++;
            break;
        case STATE_WORD1:
            if (_istspace(ch)) end = pos, state++;



        case STATE_WH2:
            if (!_istspace(ch)) state += 2;
            else end = pos, state++;
            break;
        case STATE_PAR1:
            if (ch == _T('('))
            {
                if (end == begin) end = pos;
                state += OFFSET_COMMENT;
            }
            else state += 2;
            break;
        case STATE_UP0:


        case STATE_UP1:
            if 

            else if (ch == _T('('))

            switch (ch)
            {
            case _T('('):
                state += OFFSET_COMMENT;
                comment++;
                word++;
                break;
            case _T(' '):
            case _T('\t'):
                break;
            case _T('-'):

            default:
                

            }
            break;

        case STATE_PAR0 + OFFSET_COMMENT:
            switch (ch)
            {
            case _T('('):
                comment++;
                break;
            case _T(')'):
                if (!comment) state -= OFFSET_COMMENT - 1;
                else comment--;
                break;
            }
            break;
        }

        if (upd) ch = str[++pos], upd = 0;
    }
}