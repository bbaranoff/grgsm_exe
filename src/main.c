/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * grgsm_exe - la couche 1 gr-gsm du Calypso, sans QEMU et sans ARM.
 *
 * [2026-09-16] Pendant de c54x_exe, et encore plus leger : mesure faite sur
 * l'objet compile, calypso_l1_grgsm.c ne demande que QUATRE symboles a QEMU
 * (cpu_physical_memory_rw, qemu_set_fd_handler, g_malloc, g_free) et CINQ a la
 * plateforme (calypso_api_ram, calypso_trx_get_fn, calypso_trx_autosync_fn,
 * calypso_trf6151_arfcn, calypso_trf6151_apm_for_rf).
 *
 * Ce que ca permet : faire tourner la L1 contre ses entrees reelles - les
 * segments /dev/shm/calypso_* et le flux SCH sur UDP 4731 - sans booter ARM
 * ni firmware. Donc observer ce que la L1 fait d'un burst donne, en boucle.
 *
 * Ce que ca ne permet PAS : il n'y a pas de firmware osmocom-bb pour lire
 * l'API RAM ni pour repondre. La L1 ecrit dans le vide, et c'est exactement
 * l'interet - on regarde ce qu'elle ecrit.
 *
 * Les sources ne sont pas recopiees : ce binaire compile celles de
 * /opt/GSM/qosmo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "hw/arm/calypso/calypso_api.h"

void calypso_l1_init(const char *firmware_elf);
void calypso_l1_frame_tick(void);
bool calypso_l1_si_valid(void);
uint32_t calypso_l1s_fn(void);
bool calypso_l1_read_override(uint32_t off, uint16_t *out);

/* ── ce que la plateforme fournirait ───────────────────────────────────── */
static uint16_t g_api_ram[CALYPSO_API_WORDS];
static uint32_t g_fn;
static int64_t  g_fn_offset;

uint16_t *calypso_api_ram(void) { return g_api_ram; }
uint32_t calypso_trx_get_fn(void) { return (uint32_t)((int64_t)g_fn + g_fn_offset); }

void calypso_trx_autosync_fn(uint32_t sch_fn)
{
    g_fn_offset = (int64_t)sch_fn - (int64_t)g_fn;
    printf("  [sync] SCH fn=%u -> decalage %+lld\n",
           sch_fn, (long long)g_fn_offset);
}

/* Sans ARM il n'y a pas de memoire invitee : les lectures rendent zero. */
void cpu_physical_memory_rw(uint64_t addr, void *buf, uint64_t len, bool wr)
{
    (void)addr;
    if (!wr) {
        memset(buf, 0, len);
    }
}

/* Le tap L1CTL sort par l'UART modem, qui n'existe pas ici. */
void calypso_l1ctl_tap_channel_released(void) { }

int main(int argc, char **argv)
{
    long trames = 5000;
    bool verbeux = false;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--trames") && i + 1 < argc) trames = atol(argv[++i]);
        else if (!strcmp(argv[i], "--verbeux")) verbeux = true;
        else {
            fprintf(stderr, "usage: %s [--trames N] [--verbeux]\n", argv[0]);
            return 2;
        }
    }

    printf("couche 1 gr-gsm, hors QEMU\n");
    printf("  entrees attendues : /dev/shm/calypso_*  et  SCH sur UDP 4731\n\n");

    calypso_l1_init(NULL);

    uint16_t *d_task_d = &g_api_ram[(API_R_PAGE(0) + RP_D_TASK_D) / 2];
    unsigned si_ok = 0, taches = 0;
    uint16_t prec = 0;

    for (long t = 0; t < trames; t++) {
        g_fn = (uint32_t)t;
        calypso_l1_frame_tick();

        if (calypso_l1_si_valid()) si_ok++;
        if (*d_task_d != prec) { taches++; prec = *d_task_d; }
        if (verbeux && (t % 100) == 0) {
            printf("  trame %5ld  fn=%u  l1s_fn=%u  d_task_d=0x%04x  si=%d\n",
                   t, calypso_trx_get_fn(), calypso_l1s_fn(), *d_task_d,
                   calypso_l1_si_valid());
        }
        usleep(100);   /* ~4.6 ms de trame TDMA, accelere */
    }

    printf("\n─── bilan sur %ld trames ───\n", trames);
    printf("  si_valid vrai      : %u trame(s)\n", si_ok);
    printf("  d_task_d change    : %u fois\n", taches);
    if (!si_ok) {
        printf("\n  si_valid toujours faux : aucune entree ne parvient a la L1.\n"
               "  Normal si rien ne publie dans /dev/shm/calypso_* ni sur UDP 4731.\n"
               "  C'est la que se branche un rejeu de bursts enregistres.\n");
    }
    return 0;
}
