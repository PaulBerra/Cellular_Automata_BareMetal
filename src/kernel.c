#include <stdint.h>
#include "ca.h"

// En-tête Multiboot pour GRUB
#define MULTIBOOT_MAGIC    0x1BADB002
#define MULTIBOOT_FLAGS    0
#define MULTIBOOT_CHECKSUM (-(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS))
__attribute__((section(".multiboot")))
unsigned int multiboot_hdr[] = {
    MULTIBOOT_MAGIC,
    MULTIBOOT_FLAGS,
    MULTIBOOT_CHECKSUM
};

#define LARGEUR_ECRAN   160  // Higher resolution for more detail
#define HAUTEUR_ECRAN   50   // Higher resolution for more detail

// Buffers statiques pour l'automate évolutif
static CelluleEvolutive grille_cellules_principales[LARGEUR_ECRAN * HAUTEUR_ECRAN];
static CelluleEvolutive grille_cellules_calcul[LARGEUR_ECRAN * HAUTEUR_ECRAN];
static EnvironnementLocal grille_environnement[LARGEUR_ECRAN * HAUTEUR_ECRAN];

// Pointeur vers la mémoire VGA pour l'affichage en mode texte
static volatile uint8_t *memoire_ecran_vga = (volatile uint8_t*)0xB8000;

void kmain(void) {
    // 1) Création de l’objet CA
    AutomateCellulaire mon_automate = {
        .largeur_grille              = LARGEUR_ECRAN,
        .hauteur_grille              = HAUTEUR_ECRAN,
        .regles_format_texte         = REGLES_AUTOMATE,
        .grille_cellules_actuelles   = grille_cellules_principales,
        .grille_cellules_suivantes   = grille_cellules_calcul,
        .grille_environnement        = grille_environnement,
        .generation_actuelle         = 0,
        .population_totale           = 0
    };

    // 2) Effacer l'écran (fond noir)
    for (int position = 0; position < LARGEUR_ECRAN * HAUTEUR_ECRAN; position++) {
        memoire_ecran_vga[2 * position] = ' ';
        memoire_ecran_vga[2 * position + 1] = CA_ATTR_DEAD;
    }

    // 3) Préparation de la simulation
    analyser_regles_automate(&mon_automate);                    // Analyser les règles "B3/S23"
    initialiser_grille_aleatoire(&mon_automate, 0x94215687);    // Créer une configuration naturelle aléatoire

    // 4) Boucle principale
    while (1) {
        afficher_grille_sur_ecran(&mon_automate, memoire_ecran_vga);  // Affichage sur l'écran
        calculer_generation_suivante(&mon_automate);                  // Calcul de la prochaine génération
        
        // Temporisation configurable (voir VITESSE_SIMULATION dans ca.h)
        for (volatile uint32_t compteur_delai = 0; compteur_delai < VITESSE_SIMULATION; compteur_delai++);
    }
}
