#include "ca.h"

#define NULL ((void*)0)  // Définition simple de NULL pour kernel bare-metal

// Implémentations simples des fonctions mathématiques pour kernel bare-metal
static float simple_sin(float x) {
    // Approximation de Taylor pour sin(x) : x - x³/6 + x⁵/120
    // Normalisation x dans [-π, π]
    while (x > 3.14159f) x -= 6.28318f;
    while (x < -3.14159f) x += 6.28318f;
    
    float x2 = x * x;
    return x * (1.0f - x2/6.0f + x2*x2/120.0f);
}

static float simple_abs(float x) {
    return (x < 0.0f) ? -x : x;
}

// Déclarations forward pour éviter les erreurs de compilation
static float calculer_fertilite(uint8_t age);
static uint8_t determiner_espece(CelluleEvolutive* parents[], int nombre_parents, 
                                int position_x, int position_y, int largeur, int hauteur);

/**
 * Calculates predation pressure for a given generation and position
 * Implements realistic predator-prey cycles
 */
static uint8_t calculer_pression_predation(uint32_t generation, int x, int y, int largeur, int hauteur) {
    float cycle_predation = 2.0f * 3.14159f * generation / PREDATION_CYCLE;
    float intensite_base = 0.5f + 0.5f * simple_sin(cycle_predation);
    
    // Predation gradient: higher at edges (predators hunt from outside)
    float distance_bord_x = (x < largeur/2) ? (float)x / (largeur/2) : (float)(largeur-x) / (largeur/2);
    float distance_bord_y = (y < hauteur/2) ? (float)y / (hauteur/2) : (float)(hauteur-y) / (hauteur/2);
    float distance_centre = 1.0f - (distance_bord_x + distance_bord_y) / 2.0f;
    
    float pression_finale = intensite_base * (0.3f + 0.7f * distance_centre) * PREDATION_PRESSURE;
    return (pression_finale > 255.0f) ? 255 : (uint8_t)pression_finale;
}

/**
 * Calculates epidemic disease presence based on population density
 * Realistic disease spread modeling
 */
static uint8_t calculer_pathogenes(uint32_t generation, int densite_locale) {
    float cycle_epidemie = 2.0f * 3.14159f * generation / EPIDEMIC_CYCLE;
    float intensite_epidemie = simple_abs(simple_sin(cycle_epidemie));
    
    // Disease spreads faster in dense populations (realistic epidemiology)
    float facteur_densite = (densite_locale > 4) ? 1.5f : 0.8f;
    
    float pathogenes = intensite_epidemie * facteur_densite * EPIDEMIC_MORTALITY;
    return (pathogenes > 255.0f) ? 255 : (uint8_t)pathogenes;
}

/**
 * Calculates food scarcity based on environmental cycles
 * Implements seasonal resource availability patterns
 */
static float calculer_disponibilite_nourriture(uint32_t generation) {
    float cycle_nourriture = 2.0f * 3.14159f * generation / FOOD_SCARCITY_CYCLE;
    return 0.6f + 0.4f * simple_sin(cycle_nourriture + 1.57f);  // Shifted sine for seasons
}

// -------------------------------------------------------------
// parse des règles au format "B<numeros>/S<numeros>" ex: "B3/S23"
// -------------------------------------------------------------
void analyser_regles_automate(AutomateCellulaire *automate) {
    // Vérification des pointeurs
    if (!automate || !automate->regles_format_texte) return;
    
    const char *caractere_actuel = automate->regles_format_texte;
    automate->masque_conditions_naissance = 0;  // Réinitialiser les masques
    automate->masque_conditions_survie = 0;
    
    // La règle doit commencer par 'B' (Birth = naissance)
    if (*caractere_actuel != 'B') return;
    caractere_actuel++;
    
    // Lire les chiffres après 'B' jusqu'à '/'
    // Ces chiffres indiquent combien de voisins sont nécessaires pour une naissance
    while (*caractere_actuel && *caractere_actuel != '/') {
        if (*caractere_actuel >= '0' && *caractere_actuel <= '8') {
            int nombre_voisins_pour_naissance = *caractere_actuel - '0';
            automate->masque_conditions_naissance |= (1u << nombre_voisins_pour_naissance);
        }
        caractere_actuel++;
    }
    
    // Chercher la partie survie après '/'
    if (*caractere_actuel == '/') caractere_actuel++;
    
    // La partie survie commence par 'S' (Survival = survie)
    if (*caractere_actuel == 'S') {
        caractere_actuel++;
        // Lire les chiffres après 'S'
        // Ces chiffres indiquent combien de voisins permettent la survie
        while (*caractere_actuel) {
            if (*caractere_actuel >= '0' && *caractere_actuel <= '8') {
                int nombre_voisins_pour_survie = *caractere_actuel - '0';
                automate->masque_conditions_survie |= (1u << nombre_voisins_pour_survie);
            }
            caractere_actuel++;
        }
    }
}

// -----------------------------------------------------------------
// remplit ca->grid[i] avec 0 ou 1, taille = width*height, LCG trivial
// -----------------------------------------------------------------
// =============================
// UTILITAIRES COMMUNS POUR L'INITIALISATION
// =============================

// Fonction helper pour nettoyer la grille évolutive
static void nettoyer_grille(AutomateCellulaire *automate) {
    int taille_totale = automate->largeur_grille * automate->hauteur_grille;
    
    // Nettoyer les cellules
    for (int i = 0; i < taille_totale; i++) {
        automate->grille_cellules_actuelles[i].vivante = 0;
        automate->grille_cellules_actuelles[i].age = 0;
        automate->grille_cellules_actuelles[i].genotype_survie = 128;  // Valeur par défaut
        automate->grille_cellules_actuelles[i].genotype_naissance = 128;
        automate->grille_cellules_actuelles[i].sante = 0;
        automate->grille_cellules_actuelles[i].race = RACE_EXPLORATRICE;
        automate->grille_cellules_actuelles[i].polarisation = DIRECTION_NORD;
        automate->grille_cellules_actuelles[i].force_polarisation = 0;
        automate->grille_cellules_actuelles[i].compteur_mouvement = 0;
        automate->grille_cellules_actuelles[i].fitness_reproductif = 50;
        automate->grille_cellules_actuelles[i].efficacite_energetique = 128;
        automate->grille_cellules_actuelles[i].espece_id = 0;
        automate->grille_cellules_actuelles[i].resistance_maladie = 100;
        automate->grille_cellules_actuelles[i].camouflage_predation = 100;
        automate->grille_cellules_actuelles[i].territorialite = 100;
        automate->grille_cellules_actuelles[i].adaptabilite_stress = 100;
        automate->grille_cellules_actuelles[i].generation_naissance = 0;
        
        automate->grille_cellules_suivantes[i].vivante = 0;
        automate->grille_cellules_suivantes[i].age = 0;
        automate->grille_cellules_suivantes[i].genotype_survie = 128;
        automate->grille_cellules_suivantes[i].genotype_naissance = 128;
        automate->grille_cellules_suivantes[i].sante = 0;
        automate->grille_cellules_suivantes[i].race = RACE_EXPLORATRICE;
        automate->grille_cellules_suivantes[i].polarisation = DIRECTION_NORD;
        automate->grille_cellules_suivantes[i].force_polarisation = 0;
        automate->grille_cellules_suivantes[i].compteur_mouvement = 0;
        automate->grille_cellules_suivantes[i].fitness_reproductif = 50;
        automate->grille_cellules_suivantes[i].efficacite_energetique = 128;
        automate->grille_cellules_suivantes[i].espece_id = 0;
        automate->grille_cellules_suivantes[i].resistance_maladie = 100;
        automate->grille_cellules_suivantes[i].camouflage_predation = 100;
        automate->grille_cellules_suivantes[i].territorialite = 100;
        automate->grille_cellules_suivantes[i].adaptabilite_stress = 100;
        automate->grille_cellules_suivantes[i].generation_naissance = 0;
        
        // Initialiser l'environnement
        automate->grille_environnement[i].nutriments = NUTRIMENTS_INITIAUX;
        automate->grille_environnement[i].temperature = 128;  // Valeur neutre
        automate->grille_environnement[i].pression_predation = 0;
        automate->grille_environnement[i].pathogenes_present = 0;
        automate->grille_environnement[i].toxicite_locale = 0;
        automate->grille_environnement[i].competition_territoriale = 0;
    }
}

// Fonction helper pour calculer les seuils de probabilité selon les constantes
static uint32_t calculer_seuil_probabilite(int densite_pourcentage) {
    // Convertit un pourcentage (0-100) en seuil pour générateur 32-bit
    return (0xFFFFFFFFu / 100) * densite_pourcentage;
}

// =============================
// FONCTIONS D'INITIALISATION MODULAIRES
// =============================

// Distribution uniforme simple
void initialiser_grille_uniforme(AutomateCellulaire *automate, uint32_t graine_aleatoire) {
    if (!automate || !automate->grille_cellules_actuelles) return;
    
    uint32_t generateur = (graine_aleatoire != 0) ? graine_aleatoire : 0x12345678;
    int largeur = automate->largeur_grille;
    int hauteur = automate->hauteur_grille;
    
    nettoyer_grille(automate);
    
    // Densité fixe au milieu de la plage configurée
    int densite = (DENSITE_MINIMUM + DENSITE_MAXIMUM) / 2;
    uint32_t seuil = calculer_seuil_probabilite(densite);
    
    for (int ligne = 0; ligne < hauteur; ligne++) {
        for (int colonne = 0; colonne < largeur; colonne++) {
            generateur = generateur * 1103515245u + 12345u;
            if (generateur < seuil) {
                CelluleEvolutive* cellule = &automate->grille_cellules_actuelles[ligne * largeur + colonne];
                cellule->vivante = 1;
                cellule->age = FERTILITE_DEBUT + (generateur % (FERTILITE_OPTIMALE - FERTILITE_DEBUT));
                cellule->genotype_survie = 100 + (generateur % 56);
                cellule->genotype_naissance = 100 + ((generateur >> 8) % 56);
                cellule->sante = 50;
                
                // Assignation aléatoire de race et polarisation
                cellule->race = (RaceCellule)(generateur % NOMBRE_RACES);
                cellule->polarisation = (DirectionPolarisation)((generateur >> 4) % NOMBRE_DIRECTIONS);
                cellule->force_polarisation = FORCE_POLARISATION_INITIALE + (generateur % 64);
                cellule->compteur_mouvement = generateur % RYTHME_MOUVEMENT_LENT;
                
                // Initialisation des traits évolutifs
                cellule->fitness_reproductif = 30 + (generateur % 40);  // 30-70
                cellule->efficacite_energetique = 80 + (generateur % 80);  // 80-160
                cellule->espece_id = determiner_espece(NULL, 0, colonne, ligne, largeur, hauteur);
                
                // Initialisation des traits biologiques réalistes (variations naturelles)
                generateur = generateur * 1103515245u + 12345u;
                cellule->resistance_maladie = 80 + (generateur % 50);  // 80-130
                cellule->camouflage_predation = 70 + ((generateur >> 8) % 60);  // 70-130
                cellule->territorialite = 60 + ((generateur >> 16) % 70);  // 60-130
                cellule->adaptabilite_stress = 85 + ((generateur >> 24) % 40);  // 85-125
                cellule->generation_naissance = 0;  // Génération initiale
            }
        }
    }
}

// Distribution plus dense au centre
void initialiser_grille_centre(AutomateCellulaire *automate, uint32_t graine_aleatoire) {
    if (!automate || !automate->grille_cellules_actuelles) return;
    
    uint32_t generateur = (graine_aleatoire != 0) ? graine_aleatoire : 0x12345678;
    int largeur = automate->largeur_grille;
    int hauteur = automate->hauteur_grille;
    
    nettoyer_grille(automate);
    
    for (int ligne = 0; ligne < hauteur; ligne++) {
        for (int colonne = 0; colonne < largeur; colonne++) {
            generateur = generateur * 1103515245u + 12345u;
            
            // Calcul de la distance au centre (0.0 = centre, 1.0 = bord)
            float distance_x = (float)(colonne > largeur/2 ? colonne - largeur/2 : largeur/2 - colonne) / (largeur/2);
            float distance_y = (float)(ligne > hauteur/2 ? ligne - hauteur/2 : hauteur/2 - ligne) / (hauteur/2);
            float distance_normalisee = (distance_x + distance_y) / 2.0f;
            
            // Densité qui diminue avec la distance (DENSITE_MAXIMUM au centre, DENSITE_MINIMUM aux bords)
            int densite = DENSITE_MAXIMUM - (int)((DENSITE_MAXIMUM - DENSITE_MINIMUM) * distance_normalisee);
            uint32_t seuil = calculer_seuil_probabilite(densite);
            
            if (generateur < seuil) {
                CelluleEvolutive* cellule = &automate->grille_cellules_actuelles[ligne * largeur + colonne];
                cellule->vivante = 1;
                cellule->age = FERTILITE_DEBUT + (generateur % (FERTILITE_OPTIMALE - FERTILITE_DEBUT));
                cellule->genotype_survie = 100 + (generateur % 56);
                cellule->genotype_naissance = 100 + ((generateur >> 8) % 56);
                cellule->sante = 50;
                
                // Assignation aléatoire de race et polarisation
                cellule->race = (RaceCellule)(generateur % NOMBRE_RACES);
                cellule->polarisation = (DirectionPolarisation)((generateur >> 4) % NOMBRE_DIRECTIONS);
                cellule->force_polarisation = FORCE_POLARISATION_INITIALE + (generateur % 64);
                cellule->compteur_mouvement = generateur % RYTHME_MOUVEMENT_LENT;
                
                // Initialisation des traits évolutifs
                cellule->fitness_reproductif = 30 + (generateur % 40);  // 30-70
                cellule->efficacite_energetique = 80 + (generateur % 80);  // 80-160
                cellule->espece_id = determiner_espece(NULL, 0, colonne, ligne, largeur, hauteur);
                
                // Initialisation des traits biologiques réalistes (variations naturelles)
                generateur = generateur * 1103515245u + 12345u;
                cellule->resistance_maladie = 80 + (generateur % 50);  // 80-130
                cellule->camouflage_predation = 70 + ((generateur >> 8) % 60);  // 70-130
                cellule->territorialite = 60 + ((generateur >> 16) % 70);  // 60-130
                cellule->adaptabilite_stress = 85 + ((generateur >> 24) % 40);  // 85-125
                cellule->generation_naissance = 0;  // Génération initiale
            }
        }
    }
}

// Distribution avec clustering naturel (version actuelle améliorée)
void initialiser_grille_clusters(AutomateCellulaire *automate, uint32_t graine_aleatoire) {
    if (!automate || !automate->grille_cellules_actuelles) return;
    
    uint32_t generateur_1 = (graine_aleatoire != 0) ? graine_aleatoire : 0x12345678;
    uint32_t generateur_2 = generateur_1 ^ 0x9ABCDEF0;
    int largeur = automate->largeur_grille;
    int hauteur = automate->hauteur_grille;
    
    nettoyer_grille(automate);
    
    // Utilise les constantes configurables
    uint32_t seuil_base = calculer_seuil_probabilite(DENSITE_MINIMUM);
    uint32_t variation_max = calculer_seuil_probabilite(DENSITE_MAXIMUM) - seuil_base;
    uint32_t bonus_cluster = calculer_seuil_probabilite(BONUS_CLUSTERING);
    
    for (int ligne = 0; ligne < hauteur; ligne++) {
        for (int colonne = 0; colonne < largeur; colonne++) {
            generateur_1 = generateur_1 * 1103515245u + 12345u;
            generateur_2 = generateur_2 * 1664525u + 1013904223u;
            
            // Densité variable selon la distance au centre
            int distance_centre_x = (colonne > largeur/2) ? colonne - largeur/2 : largeur/2 - colonne;
            int distance_centre_y = (ligne > hauteur/2) ? ligne - hauteur/2 : hauteur/2 - ligne;
            int distance_totale = distance_centre_x + distance_centre_y;
            
            uint32_t seuil_probabilite = seuil_base + (variation_max * (largeur + hauteur - distance_totale)) / (largeur + hauteur);
            
            // Bonus de clustering
            int voisins_vivants = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    if (dx == 0 && dy == 0) continue;
                    int vx = colonne + dx, vy = ligne + dy;
                    if (vx >= 0 && vx < largeur && vy >= 0 && vy < hauteur) {
                        if (automate->grille_cellules_actuelles[vy * largeur + vx].vivante == 1) {
                            voisins_vivants++;
                        }
                    }
                }
            }
            
            if (voisins_vivants > 0) {
                seuil_probabilite += bonus_cluster;
            }
            
            uint32_t valeur_combinee = (generateur_1 ^ (generateur_2 >> 3)) + (ligne * 7 + colonne * 11);
            if (valeur_combinee < seuil_probabilite) {
                CelluleEvolutive* cellule = &automate->grille_cellules_actuelles[ligne * largeur + colonne];
                cellule->vivante = 1;
                cellule->age = FERTILITE_DEBUT + (generateur_1 % (FERTILITE_OPTIMALE - FERTILITE_DEBUT));
                cellule->genotype_survie = 100 + (generateur_1 % 56);
                cellule->genotype_naissance = 100 + ((generateur_2 >> 8) % 56);
                cellule->sante = 50;
            }
        }
    }
}

// Fonction principale qui choisit le type d'initialisation
void initialiser_grille_selon_type(AutomateCellulaire *automate, TypeInitialisation type, uint32_t graine_aleatoire) {
    switch (type) {
        case INIT_ALEATOIRE_UNIFORME:
            initialiser_grille_uniforme(automate, graine_aleatoire);
            break;
        case INIT_ALEATOIRE_CENTRE:
            initialiser_grille_centre(automate, graine_aleatoire);
            break;
        case INIT_ALEATOIRE_CLUSTERS:
            initialiser_grille_clusters(automate, graine_aleatoire);
            break;
        default:
            initialiser_grille_clusters(automate, graine_aleatoire);  // Par défaut
            break;
    }
}

// Fonction de compatibilité (utilise clustering par défaut)
void initialiser_grille_aleatoire(AutomateCellulaire *automate, uint32_t graine_aleatoire) {
    initialiser_grille_clusters(automate, graine_aleatoire);
}


// -----------------------------------------------------------------
// calcule un pas : compte voisins, applique masques, swap buffers
// -----------------------------------------------------------------
// =============================
// FONCTIONS ÉVOLUTIVES BIOLOGIQUES
// =============================

// Calcule le fitness reproductif selon la théorie de l'évolution adaptative
static uint8_t calculer_fitness_evolutif(CelluleEvolutive* cellule, int position_x, int position_y, 
                                        uint32_t generation, int largeur, int hauteur) {
    // Fitness de base selon l'âge optimal (courbe en cloche)
    float fitness_age = calculer_fertilite(cellule->age);
    
    // Cycle énergétique sinusoïdal créant une pression de sélection variable
    float phase_environnementale = 2.0f * 3.14159f * generation / CYCLES_ENVIRONNEMENTAUX;
    float coefficient_energetique = 1.0f + 0.3f * simple_sin(phase_environnementale);
    
    // Fitness spatial : avantage selon la position (niches écologiques)
    float distance_centre_x = 2.0f * (float)position_x / largeur - 1.0f;  // -1 à 1
    float distance_centre_y = 2.0f * (float)position_y / hauteur - 1.0f;  // -1 à 1
    float niche_factor = 1.0f - 0.3f * (distance_centre_x * distance_centre_x + distance_centre_y * distance_centre_y);
    
    // Fitness racial : spécialisations évolutives
    float bonus_racial = 1.0f;
    switch (cellule->race) {
        case RACE_EXPLORATRICE:
            bonus_racial = 1.0f + 0.2f * (1.0f - niche_factor);  // Avantage en périphérie
            break;
        case RACE_COLONISATRICE: 
            bonus_racial = 1.0f + 0.2f * niche_factor;  // Avantage au centre
            break;
        case RACE_NOMADE:
            bonus_racial = 1.0f + 0.1f * coefficient_energetique;  // Avantage lors de cycles favorables
            break;
        case RACE_ADAPTATIVE:
            bonus_racial = 1.0f + 0.15f * simple_abs(simple_sin(phase_environnementale * 2.0f));  // Avantage lors de changements
            break;
        case NOMBRE_RACES:
        default:
            bonus_racial = 1.0f;
            break;
    }
    
    // Efficacité énergétique : utilisation optimale des ressources
    float efficacite = (float)cellule->efficacite_energetique / 255.0f;
    float bonus_efficacite = 1.0f + 0.25f * efficacite * coefficient_energetique;
    
    // Calcul final du fitness (0-255)
    float fitness_total = FITNESS_AMPLITUDE * fitness_age * coefficient_energetique * 
                         niche_factor * bonus_racial * bonus_efficacite;
    
    return (fitness_total > 255.0f) ? 255 : (uint8_t)fitness_total;
}

// Calcule la distance génétique entre deux cellules pour la spéciation (réservé pour extensions futures)
/*
static uint8_t calculer_distance_genetique(CelluleEvolutive* cellule1, CelluleEvolutive* cellule2) {
    int diff_survie = int_abs((int)cellule1->genotype_survie - (int)cellule2->genotype_survie);
    int diff_naissance = int_abs((int)cellule1->genotype_naissance - (int)cellule2->genotype_naissance);
    int diff_efficacite = int_abs((int)cellule1->efficacite_energetique - (int)cellule2->efficacite_energetique);
    int diff_polarisation = int_abs((int)cellule1->force_polarisation - (int)cellule2->force_polarisation);
    
    return (diff_survie + diff_naissance + diff_efficacite + diff_polarisation) / 4;
}
*/

// Détermine l'espèce selon la distance génétique et l'environnement local
static uint8_t determiner_espece(CelluleEvolutive* parents[], int nombre_parents, 
                                int position_x, int position_y, int largeur, int hauteur) {
    if (nombre_parents == 0) return 0;
    
    // Espèce basée sur la niche écologique spatiale
    float ratio_x = (float)position_x / largeur;
    float ratio_y = (float)position_y / hauteur;
    
    // 4 zones écologiques principales créant une pression de spéciation
    if (ratio_x < 0.5f && ratio_y < 0.5f) return 1;      // Nord-Ouest
    else if (ratio_x >= 0.5f && ratio_y < 0.5f) return 2;  // Nord-Est  
    else if (ratio_x < 0.5f && ratio_y >= 0.5f) return 3;  // Sud-Ouest
    else return 4;                                        // Sud-Est
}

// Obtient les coordonnées d'une direction relative
static void obtenir_coordonnees_direction(DirectionPolarisation direction, int* delta_x, int* delta_y) {
    switch (direction) {
        case DIRECTION_NORD:       *delta_x = 0;  *delta_y = -1; break;
        case DIRECTION_NORD_EST:   *delta_x = 1;  *delta_y = -1; break;
        case DIRECTION_EST:        *delta_x = 1;  *delta_y = 0;  break;
        case DIRECTION_SUD_EST:    *delta_x = 1;  *delta_y = 1;  break;
        case DIRECTION_SUD:        *delta_x = 0;  *delta_y = 1;  break;
        case DIRECTION_SUD_OUEST:  *delta_x = -1; *delta_y = 1;  break;
        case DIRECTION_OUEST:      *delta_x = -1; *delta_y = 0;  break;
        case DIRECTION_NORD_OUEST: *delta_x = -1; *delta_y = -1; break;
        default:                   *delta_x = 0;  *delta_y = 0;  break;
    }
}

// Détermine si une cellule doit bouger selon sa race et ses paramètres
static int doit_se_deplacer(CelluleEvolutive* cellule, int nombre_voisins) {
    switch (cellule->race) {
        case RACE_EXPLORATRICE:
            // Se déplace plus souvent quand il y a peu de voisins
            return (nombre_voisins <= 2) && (cellule->compteur_mouvement % RYTHME_MOUVEMENT_RAPIDE == 0);
            
        case RACE_COLONISATRICE:
            // Se déplace rarement, préfère rester en groupe
            return (nombre_voisins == 0) && (cellule->compteur_mouvement % RYTHME_MOUVEMENT_LENT == 0);
            
        case RACE_NOMADE:
            // Se déplace constamment
            return (cellule->compteur_mouvement % RYTHME_MOUVEMENT_RAPIDE == 0);
            
        case RACE_ADAPTATIVE:
            // Se déplace selon les conditions : fuit la surpopulation, cherche les zones moyennement peuplées
            return (nombre_voisins > 4 || nombre_voisins == 0) && 
                   (cellule->compteur_mouvement % (RYTHME_MOUVEMENT_RAPIDE + 1) == 0);
            
        default:
            return 0;
    }
}

// Calcule la fertilité d'une cellule selon son âge
static float calculer_fertilite(uint8_t age) {
    if (age < FERTILITE_DEBUT) return 0.0f;
    if (age >= AGE_MAXIMUM) return 0.0f;
    
    if (age <= FERTILITE_OPTIMALE) {
        // Montée progressive de 0 à 1
        return (float)(age - FERTILITE_DEBUT) / (FERTILITE_OPTIMALE - FERTILITE_DEBUT);
    } else if (age <= FERTILITE_DECLIN) {
        // Plateau optimal
        return 1.0f;
    } else {
        // Déclin progressif
        return 1.0f - (float)(age - FERTILITE_DECLIN) / (AGE_MAXIMUM - FERTILITE_DECLIN);
    }
}

// Calcule la race héritée avec possibilité de mixité génétique
static RaceCellule calculer_race_herite(CelluleEvolutive* parents[], int nombre_parents, uint32_t* generateur) {
    if (nombre_parents == 0) return RACE_EXPLORATRICE;
    
    // Vérifier s'il y a mixité génétique (différentes races parmi les parents)
    RaceCellule race_dominante = parents[0]->race;
    int mixite_presente = 0;
    
    for (int i = 1; i < nombre_parents; i++) {
        if (parents[i]->race != race_dominante) {
            mixite_presente = 1;
            break;
        }
    }
    
    *generateur = *generateur * 1103515245u + 12345u;
    
    if (mixite_presente && ((*generateur % 100) < MIXITE_GENETIQUE_CHANCE)) {
        // Création d'une race hybride adaptative
        return RACE_ADAPTATIVE;
    } else if ((*generateur % 100) < HERITAGE_RACE_PROBABILITE) {
        // Héritage normal de la race dominante
        return race_dominante;
    } else {
        // Mutation vers une race aléatoire
        return (RaceCellule)((*generateur >> 8) % NOMBRE_RACES);
    }
}

// Calcule la polarisation héritée avec variations
static DirectionPolarisation calculer_polarisation_herite(CelluleEvolutive* parents[], int nombre_parents, uint32_t* generateur) {
    if (nombre_parents == 0) return DIRECTION_NORD;
    
    // Moyenne des polarisations parentales avec variation
    int somme_directions = 0;
    for (int i = 0; i < nombre_parents; i++) {
        somme_directions += parents[i]->polarisation;
    }
    int direction_moyenne = somme_directions / nombre_parents;
    
    // Variation génétique de la direction
    *generateur = *generateur * 1103515245u + 12345u;
    int variation = ((*generateur % 5) - 2);  // -2 à +2
    direction_moyenne = (direction_moyenne + variation + NOMBRE_DIRECTIONS) % NOMBRE_DIRECTIONS;
    
    return (DirectionPolarisation)direction_moyenne;
}

// Calcule l'âge initial d'une cellule née de plusieurs parents
static uint8_t calculer_age_herite(CelluleEvolutive* parents[], int nombre_parents, uint32_t* generateur) {
    if (nombre_parents == 0) return 0;
    
    // Moyenne des âges parentaux
    uint32_t somme_ages = 0;
    for (int i = 0; i < nombre_parents; i++) {
        somme_ages += parents[i]->age;
    }
    uint32_t age_moyen_parents = somme_ages / nombre_parents;
    
    // Héritage partiel selon facteur génétique
    uint32_t age_herite = (age_moyen_parents * FACTEUR_HEREDITE) / 100;
    
    // Mutation génétique (variation aléatoire)
    *generateur = *generateur * 1103515245u + 12345u;
    if ((*generateur % 100) < TAUX_MUTATION) {
        int mutation = ((*generateur >> 8) % (2 * VARIATION_MUTATION + 1)) - VARIATION_MUTATION;
        age_herite = (age_herite + mutation < 0) ? 0 : age_herite + mutation;
    }
    
    return (age_herite > 255) ? 255 : (uint8_t)age_herite;
}


/**
 * Updates environmental factors with realistic biological cycles
 * Implements predation, disease, food scarcity, and territorial pressure
 */
static void mettre_a_jour_environnement(AutomateCellulaire *automate) {
    int largeur = automate->largeur_grille;
    int hauteur = automate->hauteur_grille;
    uint32_t generation = automate->generation_actuelle;
    
    // Calculate global environmental factors
    float disponibilite_nourriture = calculer_disponibilite_nourriture(generation);
    
    for (int ligne = 0; ligne < hauteur; ligne++) {
        for (int colonne = 0; colonne < largeur; colonne++) {
            int position = ligne * largeur + colonne;
            EnvironnementLocal* env = &automate->grille_environnement[position];
            
            // Count local population density for realistic environmental pressure
            int densite_locale = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int vx = (colonne + dx + largeur) % largeur;
                    int vy = (ligne + dy + hauteur) % hauteur;
                    if (automate->grille_cellules_actuelles[vy * largeur + vx].vivante) {
                        densite_locale++;
                    }
                }
            }
            
            // Update nutrient availability based on seasonal cycles
            int nutriments_max = (int)(NUTRIMENTS_INITIAUX * disponibilite_nourriture);
            if (env->nutriments < nutriments_max) {
                env->nutriments += REGENERATION_NUTRIMENTS;
                if (env->nutriments > nutriments_max) {
                    env->nutriments = nutriments_max;
                }
            } else if (env->nutriments > nutriments_max) {
                env->nutriments -= 1;  // Gradual decrease during scarcity
            }
            
            // Update predation pressure (realistic predator-prey dynamics)
            env->pression_predation = calculer_pression_predation(generation, colonne, ligne, largeur, hauteur);
            
            // Update disease presence (epidemiological modeling)
            env->pathogenes_present = calculer_pathogenes(generation, densite_locale);
            
            // Territorial competition increases with local density
            env->competition_territoriale = (densite_locale > MIGRATION_PRESSURE_THRESHOLD) ? 
                                          (densite_locale * TERRITORIAL_COMPETITION) : 0;
            
            // Environmental toxicity (pollution from overcrowding)
            if (densite_locale > 6) {
                env->toxicite_locale = (densite_locale - 6) * 20;
            } else {
                env->toxicite_locale = (env->toxicite_locale > 0) ? env->toxicite_locale - 5 : 0;
            }
        }
    }
}

/**
 * Calculates next generation with advanced biological realism
 * 
 * This function implements a comprehensive evolutionary simulation including:
 * - Realistic predator-prey dynamics with spatial gradients
 * - Epidemic disease spread and resistance evolution
 * - Environmental stress adaptation and mutation
 * - Seasonal resource cycles and territorial competition
 * - Multi-trait inheritance with stress-adaptive mutation rates
 * 
 * @param automate Pointer to the cellular automaton structure
 * @note This function prevents evolutionary stagnation through realistic biological pressures
 */
void calculer_generation_suivante(AutomateCellulaire *automate) {
    // Safety checks
    if (!automate || !automate->grille_cellules_actuelles || !automate->grille_cellules_suivantes) return;
    
    int largeur = automate->largeur_grille, hauteur = automate->hauteur_grille;
    uint32_t generateur = automate->generation_actuelle * 0x9E3779B9;  // Graine basée sur génération
    automate->population_totale = 0;
    
    // 1) Mettre à jour l'environnement
    mettre_a_jour_environnement(automate);
    
    // 2) Calculer le nouvel état pour chaque cellule
    for (int ligne = 0; ligne < hauteur; ligne++) {
        for (int colonne = 0; colonne < largeur; colonne++) {
            int position_cellule = ligne * largeur + colonne;
            CelluleEvolutive* cellule_actuelle = &automate->grille_cellules_actuelles[position_cellule];
            CelluleEvolutive* cellule_suivante = &automate->grille_cellules_suivantes[position_cellule];
            EnvironnementLocal* environnement = &automate->grille_environnement[position_cellule];
            
            // Initialiser la cellule suivante comme morte
            cellule_suivante->vivante = 0;
            cellule_suivante->age = 0;
            cellule_suivante->genotype_survie = 0;
            cellule_suivante->genotype_naissance = 0;
            cellule_suivante->sante = 0;
            
            // Collecter les voisins vivants et leurs propriétés
            CelluleEvolutive* voisins_parents[8];
            int nombre_voisins_vivants = 0;
            int nombre_parents_fertiles = 0;
            
            for (int decalage_ligne = -1; decalage_ligne <= 1; decalage_ligne++) {
                for (int decalage_colonne = -1; decalage_colonne <= 1; decalage_colonne++) {
                    if (decalage_ligne == 0 && decalage_colonne == 0) continue;
                    
                    int ligne_voisin = (ligne + decalage_ligne + hauteur) % hauteur;
                    int colonne_voisin = (colonne + decalage_colonne + largeur) % largeur;
                    CelluleEvolutive* voisin = &automate->grille_cellules_actuelles[ligne_voisin * largeur + colonne_voisin];
                    
                    if (voisin->vivante) {
                        nombre_voisins_vivants++;
                        
                        // Vérifier la fertilité du voisin (seuil plus permissif)
                        float fertilite = calculer_fertilite(voisin->age);
                        if (fertilite > 0.1f && nombre_parents_fertiles < 8) {  // Seuil réduit
                            voisins_parents[nombre_parents_fertiles] = voisin;
                            nombre_parents_fertiles++;
                        }
                    }
                }
            }
            
            if (cellule_actuelle->vivante) {
                // ===== CELLULE VIVANTE : SURVIE ? =====
                
                // VIEILLISSEMENT ACCÉLÉRÉ POUR EMPÊCHER STABILITÉ
                // Vieillissement normal
                uint8_t increment_age = 1;
                
                // Accélération du vieillissement pour les cellules anciennes
                if (cellule_actuelle->age > ACCELERATION_VIEILLISSEMENT) {
                    increment_age = FACTEUR_ACCELERATION;
                }
                
                cellule_suivante->age = cellule_actuelle->age + increment_age;
                
                // Mort de vieillesse
                if (cellule_suivante->age >= AGE_MAXIMUM) {
                    continue;  // Reste morte
                }
                
                // Consommation de base
                int consommation_base = CONSOMMATION_NUTRIMENTS;
                
                // Compétition naturelle pour les ressources (biologie réaliste)
                if (nombre_voisins_vivants >= SEUIL_COMPETITION) {
                    // En cas de compétition, chaque cellule accède à moins de ressources
                    int ressources_disponibles = environnement->nutriments / (1 + nombre_voisins_vivants / 2);
                    if (ressources_disponibles >= consommation_base) {
                        environnement->nutriments -= consommation_base;
                        cellule_suivante->sante = cellule_actuelle->sante; // Stable mais pas d'amélioration
                        // Léger stress de compétition
                        if (cellule_suivante->sante > STRESS_COMPETITION) {
                            cellule_suivante->sante -= STRESS_COMPETITION;
                        }
                    } else {
                        // Ressources insuffisantes en compétition
                        cellule_suivante->sante = (cellule_actuelle->sante > 3) ? cellule_actuelle->sante - 3 : 0;
                    }
                } else {
                    // Pas de compétition : croissance normale
                    if (environnement->nutriments >= consommation_base) {
                        environnement->nutriments -= consommation_base;
                        cellule_suivante->sante = (cellule_actuelle->sante < 100) ? cellule_actuelle->sante + 1 : 100;
                    } else {
                        // Malnutrition légère
                        cellule_suivante->sante = (cellule_actuelle->sante > 2) ? cellule_actuelle->sante - 2 : 0;
                    }
                }
                
                // Vieillissement naturel (perte progressive avec l'âge)
                if (cellule_suivante->age > FERTILITE_DECLIN) {
                    int perte_age = (cellule_suivante->age - FERTILITE_DECLIN) / 20;  // Vieillissement progressif
                    cellule_suivante->sante = (cellule_suivante->sante > perte_age) ? 
                                            cellule_suivante->sante - perte_age : 0;
                }
                
                // === REALISTIC BIOLOGICAL SURVIVAL FACTORS ===
                
                // Disease mortality check (epidemiological realism)
                if (environnement->pathogenes_present > 0) {
                    generateur = generateur * 1103515245u + 12345u;
                    float resistance_disease = (float)cellule_actuelle->resistance_maladie / 255.0f;
                    float risk_disease = (float)environnement->pathogenes_present / 255.0f;
                    float survival_probability = resistance_disease / (risk_disease + 0.1f);
                    
                    if ((float)(generateur % 1000) / 1000.0f > survival_probability) {
                        continue;  // Death by disease
                    }
                }
                
                // Predation mortality check (predator-prey dynamics)
                if (environnement->pression_predation > 0) {
                    generateur = generateur * 1103515245u + 12345u;
                    float camouflage_effectiveness = (float)cellule_actuelle->camouflage_predation / 255.0f;
                    float predation_risk = (float)environnement->pression_predation / 255.0f;
                    float escape_probability = camouflage_effectiveness;
                    
                    if ((float)(generateur % 1000) / 1000.0f > escape_probability && predation_risk > 0.2f) {
                        continue;  // Death by predation
                    }
                }
                
                // Environmental toxicity effects
                if (environnement->toxicite_locale > 100) {
                    cellule_suivante->sante = (cellule_suivante->sante > 2) ? cellule_suivante->sante - 2 : 0;
                }
                
                // Basic malnutrition check (more permissive)
                if (cellule_suivante->sante < 1) {
                    continue;  // Death by starvation
                }
                
                // INSTABILITÉ GÉNÉTIQUE PROGRESSIVE 
                // L'instabilité augmente avec l'âge et les générations pour empêcher les structures stables
                uint32_t instabilite_totale = 0;
                if (cellule_suivante->age > SEUIL_INSTABILITE_AGE) {
                    instabilite_totale += (cellule_suivante->age - SEUIL_INSTABILITE_AGE) / 10;
                }
                instabilite_totale += (automate->generation_actuelle * INSTABILITE_GENERATION) / 10000;  // Très réduit
                
                // Chance de mutation spontanée progressive (très réduite)
                generateur = generateur * 1103515245u + 12345u;
                if ((generateur % 1000) < instabilite_totale) {  // Changé de % 100 à % 1000 pour réduire drastiquement
                    // Instabilité : survie/mort aléatoire qui brise les patterns stables
                    if ((generateur >> 8) % 100 < 10) {  // Réduit de 30% à 10% de chance de mort spontanée
                        continue;  // Mort par instabilité génétique
                    }
                }
                
                // MORTALITÉ FORCÉE PAR HAUTE DENSITÉ LOCALE
                // Empêche les blocs stables en forçant la mort en zones denses
                if (nombre_voisins_vivants >= SEUIL_DENSITE_FATALE) {
                    generateur = generateur * 1103515245u + 12345u;
                    if ((generateur % 100) < CHANCE_MORT_DENSITE) {
                        continue;  // Mort par surpopulation locale
                    }
                }
                
                // Application des règles de survie modifiées par génotype
                uint16_t masque_survie_adapte = automate->masque_conditions_survie;
                // Modification légère selon génotype (rend certaines cellules plus résistantes)
                if (cellule_actuelle->genotype_survie > 128) {
                    masque_survie_adapte |= (1u << (nombre_voisins_vivants + 1));  // Tolère un voisin de plus
                } else if (cellule_actuelle->genotype_survie < 64) {
                    masque_survie_adapte &= ~(1u << (nombre_voisins_vivants - 1));  // Tolère un voisin de moins
                }
                
                if (masque_survie_adapte & (1u << nombre_voisins_vivants)) {
                    // Survie !
                    cellule_suivante->vivante = 1;
                    cellule_suivante->genotype_survie = cellule_actuelle->genotype_survie;
                    cellule_suivante->genotype_naissance = cellule_actuelle->genotype_naissance;
                    
                    // Conservation des propriétés de race et mouvement
                    cellule_suivante->race = cellule_actuelle->race;
                    cellule_suivante->polarisation = cellule_actuelle->polarisation;
                    cellule_suivante->force_polarisation = cellule_actuelle->force_polarisation;
                    cellule_suivante->compteur_mouvement = cellule_actuelle->compteur_mouvement + 1;
                    
                    // Conservation des traits évolutifs et biologiques
                    cellule_suivante->fitness_reproductif = cellule_actuelle->fitness_reproductif;
                    cellule_suivante->efficacite_energetique = cellule_actuelle->efficacite_energetique;
                    cellule_suivante->espece_id = cellule_actuelle->espece_id;
                    cellule_suivante->resistance_maladie = cellule_actuelle->resistance_maladie;
                    cellule_suivante->camouflage_predation = cellule_actuelle->camouflage_predation;
                    cellule_suivante->territorialite = cellule_actuelle->territorialite;
                    cellule_suivante->adaptabilite_stress = cellule_actuelle->adaptabilite_stress;
                    cellule_suivante->generation_naissance = cellule_actuelle->generation_naissance;
                    
                    automate->population_totale++;
                }
                
            } else {
                // ===== CELLULE MORTE : NAISSANCE ? =====
                
                // CONDITIONS ÉVOLUTIVES DE REPRODUCTION AVEC FITNESS DIFFÉRENTIEL
                if (nombre_parents_fertiles >= 1 && environnement->nutriments >= (CONSOMMATION_NUTRIMENTS * 2)) {
                    
                    // Calcul du fitness moyen des parents (pression de sélection)
                    float fitness_total = 0.0f;
                    float fertilite_total = 0.0f;
                    
                    for (int i = 0; i < nombre_parents_fertiles; i++) {
                        float fertilite = calculer_fertilite(voisins_parents[i]->age);
                        uint8_t fitness_parent = calculer_fitness_evolutif(voisins_parents[i], 
                                                                         colonne, ligne, 
                                                                         automate->generation_actuelle,
                                                                         largeur, hauteur);
                        fitness_total += (float)fitness_parent / 255.0f;
                        fertilite_total += fertilite;
                    }
                    
                    float fitness_moyen = fitness_total / nombre_parents_fertiles;
                    float probabilite_naissance = fertilite_total / nombre_parents_fertiles;
                    
                    // Bonus de fitness : meilleurs parents = plus de descendants
                    probabilite_naissance *= (0.5f + 0.5f * fitness_moyen);
                    
                    // Appliquer les règles de naissance
                    generateur = generateur * 1103515245u + 12345u;
                    float seuil_naissance = (float)(generateur % 1000) / 1000.0f;
                    
                    if ((automate->masque_conditions_naissance & (1u << nombre_voisins_vivants)) && 
                        seuil_naissance < probabilite_naissance) {
                        
                        // NAISSANCE avec dispersion !
                        cellule_suivante->vivante = 1;
                        
                        // Héritage de l'âge des parents avec moins de pénalité
                        cellule_suivante->age = calculer_age_herite(voisins_parents, nombre_parents_fertiles, &generateur);
                        
                        // HÉRITAGE DE RACE ET POLARISATION
                        cellule_suivante->race = calculer_race_herite(voisins_parents, nombre_parents_fertiles, &generateur);
                        cellule_suivante->polarisation = calculer_polarisation_herite(voisins_parents, nombre_parents_fertiles, &generateur);
                        cellule_suivante->force_polarisation = FORCE_POLARISATION_INITIALE + (generateur % 64);
                        cellule_suivante->compteur_mouvement = 0;
                        
                        // HÉRITAGE DES TRAITS ÉVOLUTIFS AVEC MUTATIONS
                        // Fitness reproductif : moyenne des parents + mutation
                        uint32_t fitness_herite = 0;
                        uint32_t efficacite_herite = 0;
                        for (int i = 0; i < nombre_parents_fertiles; i++) {
                            fitness_herite += voisins_parents[i]->fitness_reproductif;
                            efficacite_herite += voisins_parents[i]->efficacite_energetique;
                        }
                        fitness_herite /= nombre_parents_fertiles;
                        efficacite_herite /= nombre_parents_fertiles;
                        
                        // === REALISTIC EVOLUTIONARY MUTATIONS WITH STRESS ADAPTATION ===
                        
                        // Calculate environmental stress level for mutation rate adaptation
                        float stress_level = 0.0f;
                        stress_level += (float)environnement->pathogenes_present / 255.0f * 0.3f;
                        stress_level += (float)environnement->pression_predation / 255.0f * 0.4f;
                        stress_level += (float)environnement->toxicite_locale / 255.0f * 0.2f;
                        stress_level += (nombre_voisins_vivants > MIGRATION_PRESSURE_THRESHOLD) ? 0.1f : 0.0f;
                        
                        // Adaptive mutation rate: higher under stress (realistic biological response)
                        uint32_t taux_mutation_adaptatif = BASE_MUTATION_RATE + 
                                                          (uint32_t)(stress_level * STRESS_MUTATION_MULTIPLIER);
                        
                        // Fitness evolution with stress-adaptive mutations
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < taux_mutation_adaptatif) {
                            int mutation_fitness = ((generateur >> 8) % 21) - 10;  // -10 à +10
                            fitness_herite = (fitness_herite + mutation_fitness < 0) ? 0 : 
                                           (fitness_herite + mutation_fitness > 255) ? 255 : 
                                           fitness_herite + mutation_fitness;
                        }
                        
                        // Energy efficiency evolution
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < taux_mutation_adaptatif) {
                            int mutation_efficacite = ((generateur >> 8) % 21) - 10;
                            efficacite_herite = (efficacite_herite + mutation_efficacite < 0) ? 0 : 
                                              (efficacite_herite + mutation_efficacite > 255) ? 255 : 
                                              efficacite_herite + mutation_efficacite;
                        }
                        
                        // === BIOLOGICAL TRAIT INHERITANCE WITH EVOLUTION ===
                        
                        // Disease resistance inheritance (crucial for epidemic survival)
                        uint32_t resistance_moyenne = 0;
                        for (int i = 0; i < nombre_parents_fertiles; i++) {
                            resistance_moyenne += voisins_parents[i]->resistance_maladie;
                        }
                        resistance_moyenne /= nombre_parents_fertiles;
                        
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < RESISTANCE_EVOLUTION_RATE) {
                            int mutation_resistance = ((generateur >> 8) % 31) - 15;  // -15 to +15
                            resistance_moyenne = (resistance_moyenne + mutation_resistance < 0) ? 0 :
                                               (resistance_moyenne + mutation_resistance > 255) ? 255 :
                                               resistance_moyenne + mutation_resistance;
                        }
                        
                        // Predation camouflage inheritance
                        uint32_t camouflage_moyen = 0;
                        for (int i = 0; i < nombre_parents_fertiles; i++) {
                            camouflage_moyen += voisins_parents[i]->camouflage_predation;
                        }
                        camouflage_moyen /= nombre_parents_fertiles;
                        
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < taux_mutation_adaptatif) {
                            int mutation_camouflage = ((generateur >> 8) % 21) - 10;
                            camouflage_moyen = (camouflage_moyen + mutation_camouflage < 0) ? 0 :
                                             (camouflage_moyen + mutation_camouflage > 255) ? 255 :
                                             camouflage_moyen + mutation_camouflage;
                        }
                        
                        // Set all inherited traits
                        cellule_suivante->fitness_reproductif = (uint8_t)fitness_herite;
                        cellule_suivante->efficacite_energetique = (uint8_t)efficacite_herite;
                        cellule_suivante->resistance_maladie = (uint8_t)resistance_moyenne;
                        cellule_suivante->camouflage_predation = (uint8_t)camouflage_moyen;
                        cellule_suivante->territorialite = (voisins_parents[0]->territorialite + 
                                                          ((generateur % 21) - 10)) % 256;
                        cellule_suivante->adaptabilite_stress = (voisins_parents[0]->adaptabilite_stress + 
                                                               ((generateur >> 8) % 21) - 10) % 256;
                        cellule_suivante->generation_naissance = (uint8_t)(automate->generation_actuelle % 256);
                        cellule_suivante->espece_id = determiner_espece(voisins_parents, nombre_parents_fertiles, 
                                                                       colonne, ligne, largeur, hauteur);
                        
                        // DISPERSION : Les descendants ont tendance à éviter la surpopulation
                        // En zone dense, réduire la probabilité de reproduction
                        if (nombre_voisins_vivants >= 3) {
                            generateur = generateur * 1103515245u + 12345u;
                            if ((generateur % 100) < 60) {  // 60% de chance d'échec en zone dense
                                cellule_suivante->vivante = 0;
                                continue;
                            }
                        }
                        
                        // Héritage génétique avec diversification forcée
                        uint8_t genotype_moyen_survie = 0;
                        uint8_t genotype_moyen_naissance = 0;
                        for (int i = 0; i < nombre_parents_fertiles; i++) {
                            genotype_moyen_survie += voisins_parents[i]->genotype_survie;
                            genotype_moyen_naissance += voisins_parents[i]->genotype_naissance;
                        }
                        genotype_moyen_survie /= nombre_parents_fertiles;
                        genotype_moyen_naissance /= nombre_parents_fertiles;
                        
                        // Augmenter les mutations en zones de compétition pour favoriser l'adaptation
                        uint32_t taux_mutation_local = TAUX_MUTATION;
                        if (nombre_voisins_vivants >= SEUIL_COMPETITION) {
                            taux_mutation_local *= 2;  // Double mutation en compétition
                        }
                        
                        // Application avec taux adaptatif
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < taux_mutation_local) {
                            int mutation_survie = ((generateur >> 8) % 41) - 20;  // -20 à +20
                            int mutation_naissance = ((generateur >> 16) % 41) - 20;
                            genotype_moyen_survie = (genotype_moyen_survie + mutation_survie < 0) ? 0 : 
                                                  (genotype_moyen_survie + mutation_survie > 255) ? 255 : 
                                                  genotype_moyen_survie + mutation_survie;
                            genotype_moyen_naissance = (genotype_moyen_naissance + mutation_naissance < 0) ? 0 : 
                                                     (genotype_moyen_naissance + mutation_naissance > 255) ? 255 : 
                                                     genotype_moyen_naissance + mutation_naissance;
                        }
                        
                        cellule_suivante->genotype_survie = genotype_moyen_survie;
                        cellule_suivante->genotype_naissance = genotype_moyen_naissance;
                        
                        // Santé initiale
                        cellule_suivante->sante = 50;  // Commence en bonne santé
                        
                        // Consommer les nutriments pour la naissance (coût réaliste)
                        environnement->nutriments -= (CONSOMMATION_NUTRIMENTS * 2);
                        
                        automate->population_totale++;
                    }
                }
            }
        }
    }
    
    // 3) Échanger les grilles de cellules
    CelluleEvolutive *grille_temporaire = automate->grille_cellules_actuelles;
    automate->grille_cellules_actuelles = automate->grille_cellules_suivantes;
    automate->grille_cellules_suivantes = grille_temporaire;
    
    // 4) PHASE DE MOUVEMENT POLARISÉ (RÉACTIVÉ AVEC PRUDENCE)
    // Mouvement très occasionnel pour introduire de la dynamique sans déstabiliser
    if (automate->generation_actuelle % 10 == 0) {  // Seulement toutes les 10 générations
        for (int ligne = 0; ligne < hauteur; ligne++) {
            for (int colonne = 0; colonne < largeur; colonne++) {
                int position_cellule = ligne * largeur + colonne;
                CelluleEvolutive* cellule = &automate->grille_cellules_actuelles[position_cellule];
                
                if (cellule->vivante && doit_se_deplacer(cellule, 0)) {
                    // Calculer position cible selon polarisation
                    int delta_x, delta_y;
                    obtenir_coordonnees_direction(cellule->polarisation, &delta_x, &delta_y);
                    
                    int nouvelle_ligne = (ligne + delta_y + hauteur) % hauteur;
                    int nouvelle_colonne = (colonne + delta_x + largeur) % largeur;
                    int nouvelle_position = nouvelle_ligne * largeur + nouvelle_colonne;
                    
                    // Déplacer seulement si la case cible est libre
                    if (!automate->grille_cellules_actuelles[nouvelle_position].vivante) {
                        // Effectuer le déplacement avec probabilité réduite
                        generateur = generateur * 1103515245u + 12345u;
                        if ((generateur % 100) < 30) {  // Seulement 30% de chance de bouger
                            automate->grille_cellules_actuelles[nouvelle_position] = *cellule;
                            
                            // Vider l'ancienne position
                            cellule->vivante = 0;
                            cellule->age = 0;
                            cellule->sante = 0;
                            cellule->race = RACE_EXPLORATRICE;
                            cellule->polarisation = DIRECTION_NORD;
                            cellule->force_polarisation = 0;
                            cellule->compteur_mouvement = 0;
                        }
                    }
                }
            }
        }
    }
    
    // 5) Incrémenter le compteur de génération
    automate->generation_actuelle++;
}

// -------------------------------------------------------------
// affiche 'O' pour vivant, ' ' pour mort dans la mémoire VGA
// -------------------------------------------------------------
// Affiche la grille : 'O' pour cellule vivante, ' ' pour cellule morte
// Fonction pour obtenir une couleur style matplotlib selon l'âge (palette viridis-like)
static uint8_t obtenir_couleur_age(uint8_t age) {
    // Normalisation de l'âge sur 0-255
    float ratio = (float)age / AGE_MAXIMUM;
    
    if (ratio < 0.2f) return 0x01;      // Bleu foncé (jeune)
    else if (ratio < 0.4f) return 0x03; // Cyan (adolescent)
    else if (ratio < 0.6f) return 0x0A; // Vert clair (adulte)
    else if (ratio < 0.8f) return 0x0E; // Jaune (mature)
    else return 0x0C;                   // Rouge (âgé)
}

// Fonction pour obtenir un caractère selon la race
static char obtenir_caractere_race(RaceCellule race, uint8_t sante) {
    // Caractères distincts par race
    switch (race) {
        case RACE_EXPLORATRICE:
            return (sante > 50) ? 'E' : 'e';  // Explorateurs
        case RACE_COLONISATRICE:
            return (sante > 50) ? 'C' : 'c';  // Colonisateurs
        case RACE_NOMADE:
            return (sante > 50) ? 'N' : 'n';  // Nomades
        case RACE_ADAPTATIVE:
            return (sante > 50) ? 'A' : 'a';  // Adaptatifs
        default:
            return '?';
    }
}

void afficher_grille_sur_ecran(const AutomateCellulaire *automate, volatile uint8_t *memoire_vga) {
    // High resolution display with sampling to fit 80x25 VGA
    int largeur_affichage = 80;
    int hauteur_affichage = 25;
    
    // Scale factors to sample the high resolution grid
    float echelle_x = (float)automate->largeur_grille / largeur_affichage;
    float echelle_y = (float)automate->hauteur_grille / hauteur_affichage;
    
    // Scientific matplotlib style display with continuous colors
    for (int ligne = 0; ligne < hauteur_affichage; ligne++) {
        for (int colonne = 0; colonne < largeur_affichage; colonne++) {
            // Sample the high resolution grid
            int ligne_grille = (int)(ligne * echelle_y);
            int colonne_grille = (int)(colonne * echelle_x);
            int position_grille = ligne_grille * automate->largeur_grille + colonne_grille;
            int position_ecran = ligne * 80 + colonne;
            
            CelluleEvolutive* cellule = &automate->grille_cellules_actuelles[position_grille];
            
            if (cellule->vivante) {
                // Display by race with color according to age
                char caractere = obtenir_caractere_race(cellule->race, cellule->sante);
                uint8_t couleur = obtenir_couleur_age(cellule->age);
                
                memoire_vga[2 * position_ecran] = caractere;
                memoire_vga[2 * position_ecran + 1] = couleur;
            } else {
                // Uniform simple background (no fertile zones)
                memoire_vga[2 * position_ecran] = ' ';
                memoire_vga[2 * position_ecran + 1] = 0x00;  // Black (matplotlib background)
            }
        }
    }
    
    /**
     * Scientific-style generation display with decimal precision
     * Shows generation count and population statistics
     */
    char info_gen[25];
    uint32_t gen = automate->generation_actuelle;
    uint32_t pop = automate->population_totale;
    int pos_info = 0;
    
    // Generation label
    info_gen[pos_info++] = 'G';
    info_gen[pos_info++] = 'e';
    info_gen[pos_info++] = 'n';
    info_gen[pos_info++] = ':';
    
    // Enhanced decimal conversion (up to 99999)
    if (gen >= 10000) {
        info_gen[pos_info++] = '0' + (gen / 10000);
        gen %= 10000;
    }
    if (gen >= 1000) {
        info_gen[pos_info++] = '0' + (gen / 1000);
        gen %= 1000;
    }
    if (gen >= 100) {
        info_gen[pos_info++] = '0' + (gen / 100);
        gen %= 100;
    }
    if (gen >= 10) {
        info_gen[pos_info++] = '0' + (gen / 10);
        gen %= 10;
    }
    info_gen[pos_info++] = '0' + gen;
    
    // Population separator
    info_gen[pos_info++] = ' ';
    info_gen[pos_info++] = 'P';
    info_gen[pos_info++] = ':';
    
    // Population count (shortened)
    if (pop >= 1000) {
        info_gen[pos_info++] = '0' + (pop / 1000);
        info_gen[pos_info++] = 'k';
    } else if (pop >= 100) {
        info_gen[pos_info++] = '0' + (pop / 100);
        pop %= 100;
        info_gen[pos_info++] = '0' + (pop / 10);
        info_gen[pos_info++] = '0' + (pop % 10);
    } else if (pop >= 10) {
        info_gen[pos_info++] = '0' + (pop / 10);
        info_gen[pos_info++] = '0' + (pop % 10);
    } else {
        info_gen[pos_info++] = '0' + pop;
    }
    
    // Display info at bottom right (extended for more characters)
    for (int i = 0; i < pos_info && i < 20; i++) {
        int pos_ecran_info = (hauteur_affichage - 1) * 80 + (80 - 20 + i);
        memoire_vga[2 * pos_ecran_info] = info_gen[i];
        memoire_vga[2 * pos_ecran_info + 1] = 0x0F;  // White on black
    }
}
