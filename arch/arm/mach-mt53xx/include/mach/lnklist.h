/*----------------------------------------------------------------------------*
 * Copyright Statement:                                                       *
 *                                                                            *
 *   This software/firmware and related documentation ("MediaTek Software")   *
 * are protected under international and related jurisdictions'copyright laws *
 * as unpublished works. The information contained herein is confidential and *
 * proprietary to MediaTek Inc. Without the prior written permission of       *
 * MediaTek Inc., any reproduction, modification, use or disclosure of        *
 * MediaTek Software, and information contained herein, in whole or in part,  *
 * shall be strictly prohibited.                                              *
 * MediaTek Inc. Copyright (C) 2010. All rights reserved.                     *
 *                                                                            *
 *   BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND     *
 * AGREES TO THE FOLLOWING:                                                   *
 *                                                                            *
 *   1)Any and all intellectual property rights (including without            *
 * limitation, patent, copyright, and trade secrets) in and to this           *
 * Software/firmware and related documentation ("MediaTek Software") shall    *
 * remain the exclusive property of MediaTek Inc. Any and all intellectual    *
 * property rights (including without limitation, patent, copyright, and      *
 * trade secrets) in and to any modifications and derivatives to MediaTek     *
 * Software, whoever made, shall also remain the exclusive property of        *
 * MediaTek Inc.  Nothing herein shall be construed as any transfer of any    *
 * title to any intellectual property right in MediaTek Software to Receiver. *
 *                                                                            *
 *   2)This MediaTek Software Receiver received from MediaTek Inc. and/or its *
 * representatives is provided to Receiver on an "AS IS" basis only.          *
 * MediaTek Inc. expressly disclaims all warranties, expressed or implied,    *
 * including but not limited to any implied warranties of merchantability,    *
 * non-infringement and fitness for a particular purpose and any warranties   *
 * arising out of course of performance, course of dealing or usage of trade. *
 * MediaTek Inc. does not provide any warranty whatsoever with respect to the *
 * software of any third party which may be used by, incorporated in, or      *
 * supplied with the MediaTek Software, and Receiver agrees to look only to   *
 * such third parties for any warranty claim relating thereto.  Receiver      *
 * expressly acknowledges that it is Receiver's sole responsibility to obtain *
 * from any third party all proper licenses contained in or delivered with    *
 * MediaTek Software.  MediaTek is not responsible for any MediaTek Software  *
 * releases made to Receiver's specifications or to conform to a particular   *
 * standard or open forum.                                                    *
 *                                                                            *
 *   3)Receiver further acknowledge that Receiver may, either presently       *
 * and/or in the future, instruct MediaTek Inc. to assist it in the           *
 * development and the implementation, in accordance with Receiver's designs, *
 * of certain softwares relating to Receiver's product(s) (the "Services").   *
 * Except as may be otherwise agreed to in writing, no warranties of any      *
 * kind, whether express or implied, are given by MediaTek Inc. with respect  *
 * to the Services provided, and the Services are provided on an "AS IS"      *
 * basis. Receiver further acknowledges that the Services may contain errors  *
 * that testing is important and it is solely responsible for fully testing   *
 * the Services and/or derivatives thereof before they are used, sublicensed  *
 * or distributed. Should there be any third party action brought against     *
 * MediaTek Inc. arising out of or relating to the Services, Receiver agree   *
 * to fully indemnify and hold MediaTek Inc. harmless.  If the parties        *
 * mutually agree to enter into or continue a business relationship or other  *
 * arrangement, the terms and conditions set forth herein shall remain        *
 * effective and, unless explicitly stated otherwise, shall prevail in the    *
 * event of a conflict in the terms in any agreements entered into between    *
 * the parties.                                                               *
 *                                                                            *
 *   4)Receiver's sole and exclusive remedy and MediaTek Inc.'s entire and    *
 * cumulative liability with respect to MediaTek Software released hereunder  *
 * will be, at MediaTek Inc.'s sole discretion, to replace or revise the      *
 * MediaTek Software at issue.                                                *
 *                                                                            *
 *   5)The transaction contemplated hereunder shall be construed in           *
 * accordance with the laws of Singapore, excluding its conflict of laws      *
 * principles.  Any disputes, controversies or claims arising thereof and     *
 * related thereto shall be settled via arbitration in Singapore, under the   *
 * then current rules of the International Chamber of Commerce (ICC).  The    *
 * arbitration shall be conducted in English. The awards of the arbitration   *
 * shall be final and binding upon both parties and shall be entered and      *
 * enforceable in any court of competent jurisdiction.                        *
 *---------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------
 * $RCSfile: lnklist.h,v $
 * $Revision: #1 $
 * $Date: 2015/11/10 $
 * $Author: dtvbm11 $
 *
 * Description: 
 *         This header file contains single and double link list macros.
 *---------------------------------------------------------------------------*/

#ifndef _LNKLIST_H_
#define _LNKLIST_H_

/*-----------------------------------------------------------------------------
                    include files
-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------
                    macros, defines, typedefs, enums
-----------------------------------------------------------------------------*/

/*
 *  Double link list
 *
 *          pt_head
 *         ---------------|
 *         |              v
 *  -----------           -------------           -------------
 *  |         |           |           |  pt_next  |           |  pt_next
 *  |         |           |  DLIST_   |---------->|  DLIST_   |-----> NULL
 *  | DLIST_T |           |  ENTRY_T  |           |  ENTRY_T  |
 *  |         | NULL <----|           |<----------|           |
 *  |         |  pt_prev  |           |  pt_prev  |           |
 *  -----------           -------------           -------------
 *       |                                        ^
 *       -----------------------------------------|
 *          pt_tail
 *
 */
#define DLIST_T(type)           \
    struct {                    \
        struct type *pt_head;   \
        struct type *pt_tail;   \
    }
    
#define DLIST_ENTRY_T(type)     \
    struct {                    \
        struct type *pt_prev;   \
        struct type *pt_next;   \
    }
    
#define DLIST_HEAD(q) ((q)->pt_head)

#define DLIST_TAIL(q) ((q)->pt_tail)

#define DLIST_IS_EMPTY(q) ((q)->pt_head == NULL)

#define DLIST_NEXT(ent, field)  ((ent)->field.pt_next)

#define DLIST_PREV(ent, field)  ((ent)->field.pt_prev)

#define DLIST_INIT(q)                       \
    do                                      \
    {                                       \
        (q)->pt_head = (q)->pt_tail = NULL; \
    } while(0)
 
#define DLIST_FOR_EACH(var, q, field)       \
    for ((var) = (q)->pt_head;              \
         (var);                             \
         (var) = (var)->field.pt_next)
         
#define DLIST_FOR_EACH_BACKWARD(var, q, field)  \
    for ((var) = (q)->pt_tail;                  \
         (var);                                 \
         (var) = (var)->field.pt_prev)

#define DLIST_INSERT_HEAD(ent, q, field)    \
    do                                      \
    {                                       \
        (ent)->field.pt_prev = NULL;                        \
        if (((ent)->field.pt_next = (q)->pt_head) == NULL)  \
        {                                                   \
            (q)->pt_tail = (ent);                           \
        }                                                   \
        else                                                \
        {                                                   \
            ((q)->pt_head)->field.pt_prev = (ent);          \
        }                                                   \
        (q)->pt_head = (ent);                               \
    } while(0)
    
#define DLIST_INSERT_TAIL(ent, q, field)                    \
    do                                                      \
    {                                                       \
        (ent)->field.pt_next = NULL;                        \
        if (((ent)->field.pt_prev = (q)->pt_tail) == NULL)  \
        {                                                   \
            (q)->pt_head = (ent);                           \
        }                                                   \
        else                                                \
        {                                                   \
            ((q)->pt_tail)->field.pt_next = (ent);          \
        }                                                   \
        (q)->pt_tail = (ent);                               \
    } while(0)

#define DLIST_INSERT_BEFORE(new, old, q, field)             \
    do                                                      \
    {                                                       \
        (new)->field.pt_next = (old);                       \
        if (((new)->field.pt_prev = (old)->field.pt_prev) == NULL)  \
        {                                                   \
            (q)->pt_head = (new);                           \
        }                                                   \
        else                                                \
        {                                                   \
            ((old)->field.pt_prev)->field.pt_next = (new);  \
        }                                                   \
        (old)->field.pt_prev = (new);                       \
    } while(0)


#define DLIST_INSERT_AFTER(new, old, q, field)              \
    do                                                      \
    {                                                       \
        (new)->field.pt_prev = (old);                       \
        if (((new)->field.pt_next = (old)->field.pt_next) == NULL)  \
        {                                                   \
            (q)->pt_tail = (new);                           \
        }                                                   \
        else                                                \
        {                                                   \
            ((old)->field.pt_next)->field.pt_prev = (new);  \
        }                                                   \
        (old)->field.pt_next = (new);                       \
    } while(0)
    
#define DLIST_REMOVE(ent, q, field)                         \
    do                                                      \
    {                                                       \
        if ((q)->pt_tail == (ent))                          \
        {                                                   \
            (q)->pt_tail = (ent)->field.pt_prev;            \
        }                                                   \
        else                                                \
        {                                                   \
            ((ent)->field.pt_next)->field.pt_prev = (ent)->field.pt_prev;   \
        }                                                   \
        if ((q)->pt_head == (ent))                          \
        {                                                   \
            (q)->pt_head = (ent)->field.pt_next;            \
        }                                                   \
        else                                                \
        {                                                   \
            ((ent)->field.pt_prev)->field.pt_next = (ent)->field.pt_next;   \
        }                                                   \
        (ent)->field.pt_next = NULL;                        \
        (ent)->field.pt_prev = NULL;                        \
    } while(0)

 
/*
 *  Single link list
 *
 *              pt_head
 *            ------------|
 *            |^          v
 *  -----------|          -------------           -------------
 *  |         ||          |           |  pt_next  |           |  pt_next
 *  |         ||          |  SLIST_   |---------->|  SLIST_   |-----> NULL
 *  | SLIST_T ||          |  ENTRY_T  |^          |  ENTRY_T  |
 *  |         |-----------|           |-----------|           |
 *  |         |  pt_prev  |           |  pt_prev  |           |
 *  -----------           -------------           -------------
 *
 */
#define SLIST_T(type)           \
    struct {                    \
        struct type *pt_head;   \
    }

#define SLIST_ENTRY_T(type)     \
    struct {                    \
        struct type *pt_next;   \
        struct type **pt_prev;  /* address of previous pt_next */ \
    }
    
#define SLIST_FIRST(list)       ((list)->pt_head)

#define SLIST_IS_EMPTY(list)    ((list)->pt_head == NULL)

#define	SLIST_NEXT(ent, field)	((ent)->field.pt_next)

#define SLIST_INIT(list)            \
    {                               \
        ((list)->pt_head = NULL);   \
    }

#define SLIST_FOR_EACH(var, list, field)    \
    for ((var) = (list)->pt_head;           \
         (var);                             \
         (var) = (var)->field.pt_next)
         
#define SLIST_INSERT_HEAD(new, list, field)                             \
    do                                                                  \
    {                                                                   \
        if (((new)->field.pt_next = (list)->pt_head) != NULL)           \
        {                                                               \
            ((list)->pt_head)->field.pt_prev = &((new)->field.pt_next); \
        }                                                               \
        (list)->pt_head = (new);                                        \
        (new)->field.pt_prev = &((list)->pt_head);                      \
    } while (0)
    
#define SLIST_INSERT_AFTER(new, old, field)                         \
    do                                                              \
    {                                                               \
        if (((new)->field.pt_next = (old)->field.pt_next) != NULL)  \
        {                                                           \
            ((old)->field.pt_next)->field.pt_prev = &((new)->field.pt_next); \
        }                                                           \
        (old)->field.pt_next = (new);                               \
        (new)->field.pt_prev = &((old)->field.pt_next);             \
    } while (0)
    

#define SLIST_INSERT_BEFORE(new, old, field)                        \
    do                                                              \
    {                                                               \
        (new)->field.pt_next = (old);                               \
        (new)->field.pt_prev = (old)->field.pt_prev;                \
        *((old)->field.pt_prev) = (new);                            \
        (old)->field.pt_prev = &((new)->field.pt_next);             \
    } while (0)


#define SLIST_REMOVE(ent, field)                                            \
    do                                                                      \
    {                                                                       \
        *((ent)->field.pt_prev) = (ent)->field.pt_next;                     \
        if ((ent)->field.pt_next != NULL)                                   \
        {                                                                   \
            ((ent)->field.pt_next)->field.pt_prev = (ent)->field.pt_prev;   \
        }                                                                   \
        (ent)->field.pt_next = NULL;                                        \
        (ent)->field.pt_prev = NULL;                                        \
    } while (0)


#endif /* _LNKLIST_H_ */
