# grgsm_exe — la couche 1 gr-gsm sans QEMU

Pendant de [`c54x_exe`](../c54x_exe), pour l'autre couche 1 : la démodulation
par gr-gsm, sans QEMU, sans ARM, sans firmware.

```bash
make
./grgsm_exe --trames 5000
./grgsm_exe --verbeux
```

## Pourquoi c'est encore plus léger

Mesuré sur l'objet compilé, `calypso_l1_grgsm.c` demande **4 symboles à QEMU**
(`cpu_physical_memory_rw`, `qemu_set_fd_handler`, `g_malloc`, `g_free`) et
**5 à la plateforme** (`calypso_api_ram`, `calypso_trx_get_fn`,
`calypso_trx_autosync_fn`, `calypso_trf6151_arfcn`,
`calypso_trf6151_apm_for_rf`). Le binaire fait **118 Ko**, contre 93 Mo pour
le `qemu-system-arm` qui l'héberge d'habitude.

Il ouvre ses vraies entrées, vérifiable à l'exécution :

```
$ ss -ulnp | grep 473
127.0.0.1:4730   users:(("grgsm_exe",...))    # GSMTAP
127.0.0.1:4731   users:(("grgsm_exe",...))    # SCH
```

Plus les segments `/dev/shm/calypso_*`. C'est là qu'on branche un rejeu de
bursts enregistrés.

## Ce que ça ne fait pas

Il n'y a pas de firmware osmocom-bb pour lire l'API RAM ni pour répondre : la
L1 écrit dans le vide. C'est l'intérêt — on regarde ce qu'elle écrit, sans que
trente variables se glissent entre la question et la réponse.

Sans rien qui publie sur `4731` ou dans `/dev/shm`, `si_valid` reste faux et le
bilan le dit explicitement plutôt que d'afficher des zéros muets.

**Les sources ne sont pas recopiées** : ce binaire compile celles de
`/opt/GSM/qosmo`, cales comprises (`QOSMO=... make` pour pointer ailleurs).
