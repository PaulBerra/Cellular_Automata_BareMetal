#ifndef CA_H
#define CA_H

#include <stdint.h>

// =============================
// CONFIGURATION FACILE À MODIFIER
// =============================

// Colors for VGA text mode display
#define CA_ATTR_ALIVE 0x0A   // Light green on black background (living cell)
#define CA_ATTR_DEAD  0x07   // Light gray on black background (dead cell)

// Random generation parameters
#define DENSITE_MINIMUM 15 // Minimum density of living cells (%) - reduced for survival
#define DENSITE_MAXIMUM 80  // Maximum density of living cells (%) - reduced for survival  
#define BONUS_CLUSTERING 6   // Probability bonus for clustering (%)

// Biological evolution parameters
#define AGE_MAXIMUM 1200             // Maximum cell age (longer for survival)
#define FERTILITE_DEBUT 5            // Minimum age for reproduction (earlier)
#define FERTILITE_OPTIMALE 30        // Age of maximum fertility
#define FERTILITE_DECLIN 150         // Age when decline begins (later)
#define FACTEUR_HEREDITE 5           // % of parental age inheritance (reduced for dynamism)
#define TAUX_MUTATION 8              // % chance of mutation (increased for diversity)
#define VARIATION_MUTATION 15        // Maximum amplitude of age mutations
#define INSTABILITE_GENERATION 1     // Very gentle instability (0.01% per generation)  
#define SEUIL_INSTABILITE_AGE 500    // Age when instability begins (very old cells)
#define SEUIL_DENSITE_FATALE 999     // Local density causing forced mortality (disabled)
#define CHANCE_MORT_DENSITE 0        // % chance of death in high density (disabled)
#define ACCELERATION_VIEILLISSEMENT 200 // Age when aging accelerates (increased)
#define FACTEUR_ACCELERATION 2       // Aging multiplier (reduced)
#define CONSOMMATION_NUTRIMENTS 1    // Nutrients consumed per cell/cycle (reduced)
#define REGENERATION_NUTRIMENTS 3    // Nutrients regenerated per cycle (balanced)
#define NUTRIMENTS_INITIAUX 100      // Starting nutrients per cell (increased for survival)

// Race and movement parameters
#define FORCE_POLARISATION_INITIALE 128  // Default polarization strength
#define HERITAGE_RACE_PROBABILITE 70     // % chance of inheriting parental race
#define MIXITE_GENETIQUE_CHANCE 20       // % chance of creating hybrid race
#define RYTHME_MOUVEMENT_RAPIDE 50       // Cycles between movements for fast races (reduced)
#define RYTHME_MOUVEMENT_LENT 100        // Cycles between movements for slow races (reduced)

/**
 * Advanced Biological Evolution Parameters - Realistic Ecosystem Simulation
 * Based on real evolutionary biology principles to prevent system stagnation
 */
#define FITNESS_AMPLITUDE 50             ///< Amplitude of evolutionary fitness variations
#define CYCLES_ENVIRONNEMENTAUX 150     ///< Environmental cycle period (generations)  
#define PREDATION_CYCLE 80               ///< Predation pressure cycle (generations)
#define EPIDEMIC_CYCLE 120               ///< Disease outbreak cycle (generations)
#define FOOD_SCARCITY_CYCLE 90           ///< Food scarcity cycle (generations)

// Evolutionary pressure parameters
#define BASE_MUTATION_RATE 3             ///< Base mutation rate (%)
#define STRESS_MUTATION_MULTIPLIER 4     ///< Mutation rate multiplier under stress
#define PREDATION_PRESSURE 15            ///< Predation mortality rate (%)
#define EPIDEMIC_MORTALITY 20            ///< Epidemic mortality rate (%)
#define RESISTANCE_EVOLUTION_RATE 12     ///< Rate of resistance trait evolution

// Migration and movement parameters  
#define MIGRATION_PRESSURE_THRESHOLD 6   ///< Density threshold triggering migration
#define FORCED_MIGRATION_RATE 25         ///< Probability of forced migration (%)
#define TERRITORIAL_COMPETITION 8        ///< Competition intensity for territory
#define DISPERSAL_ADVANTAGE 15           ///< Fitness advantage for dispersal behavior

// Biological parameters 
#define SEUIL_COMPETITION 5          // Number of neighbors creating resource competition  
#define STRESS_COMPETITION 2         // Slight health reduction in competition
#define FACTEUR_VIEILLISSEMENT 1     // Natural health loss with age
#define CYCLES_SAISONS 10           // Slow environmental changes (seasons)

// Simulation parameters
#define VITESSE_SIMULATION 500000000  // Delay between generations (higher = slower)

// Cellular automaton rules (easy to change)
#define REGLES_AUTOMATE "B3/S20"  // HighLife with replicators (prevents stagnation)
// Other examples:
// #define REGLES_AUTOMATE "B36/S23"    // HighLife (adds replicators)
// #define REGLES_AUTOMATE "B2/S23"     // Seeds (very chaotic)
// #define REGLES_AUTOMATE "B34/S34"    // 34 Life (different structures)

// Cell races with distinct properties
typedef enum {
    RACE_EXPLORATRICE = 0,    // Tendency to disperse
    RACE_COLONISATRICE = 1,   // Forms stable colonies
    RACE_NOMADE = 2,          // Constant movement
    RACE_ADAPTATIVE = 3,      // Changes according to environment
    NOMBRE_RACES = 4
} RaceCellule;

// Polarization directions for movement
typedef enum {
    DIRECTION_NORD = 0,       // North
    DIRECTION_NORD_EST = 1,   // Northeast
    DIRECTION_EST = 2,        // East
    DIRECTION_SUD_EST = 3,    // Southeast
    DIRECTION_SUD = 4,        // South
    DIRECTION_SUD_OUEST = 5,  // Southwest
    DIRECTION_OUEST = 6,      // West
    DIRECTION_NORD_OUEST = 7, // Northwest
    NOMBRE_DIRECTIONS = 8
} DirectionPolarisation;

// Available initialization types
typedef enum {
    INIT_ALEATOIRE_UNIFORME,      // Uniform distribution
    INIT_ALEATOIRE_CENTRE,        // Denser at center
    INIT_ALEATOIRE_CLUSTERS       // With natural clustering
} TypeInitialisation;

/**
 * Evolutionary Cell Structure - Advanced Biological Simulation
 * Contains all traits necessary for realistic evolution simulation
 */
typedef struct {
    uint8_t vivante;                    ///< Cell state: 0 = dead, 1 = alive
    uint8_t age;                        ///< Current age of the cell (0-255)
    uint8_t genotype_survie;            ///< Genetic survival threshold (individual variations)
    uint8_t genotype_naissance;         ///< Genetic birth threshold
    uint8_t sante;                      ///< Health state (affected by nutrients and age)
    RaceCellule race;                   ///< Cell race with specific properties
    DirectionPolarisation polarisation; ///< Preferred movement direction
    uint8_t force_polarisation;         ///< Polarization intensity (0-255)
    uint8_t compteur_mouvement;         ///< Movement rhythm counter
    
    // Advanced evolutionary traits
    uint8_t fitness_reproductif;        ///< Reproductive fitness (0-255)
    uint8_t efficacite_energetique;     ///< Resource utilization efficiency (0-255)
    uint8_t espece_id;                  ///< Species identifier for speciation (0-255)
    
    // Biological realism traits
    uint8_t resistance_maladie;         ///< Disease resistance level (0-255)
    uint8_t camouflage_predation;       ///< Predation avoidance ability (0-255)
    uint8_t territorialite;             ///< Territorial behavior strength (0-255)
    uint8_t adaptabilite_stress;        ///< Stress adaptation capacity (0-255)
    uint8_t generation_naissance;       ///< Generation when cell was born (for tracking lineages)
} CelluleEvolutive;

/**
 * Local Environment Structure - Dynamic Ecosystem Factors
 * Represents local environmental pressures affecting cell survival
 */
typedef struct {
    uint8_t nutriments;                 ///< Available nutrient level (0-255)
    uint8_t temperature;                ///< Environmental temperature factor
    uint8_t pression_predation;         ///< Current predation pressure (0-255)
    uint8_t pathogenes_present;         ///< Disease pathogen presence (0-255)
    uint8_t toxicite_locale;            ///< Local toxicity level (0-255)
    uint8_t competition_territoriale;   ///< Territorial competition intensity (0-255)
} EnvironnementLocal;

// Main evolutionary cellular automaton structure
typedef struct {
    int largeur_grille;                    // Number of columns in the grid
    int hauteur_grille;                    // Number of rows in the grid
    const char *regles_format_texte;       // Base rules in "B3/S23" format
    uint16_t masque_conditions_naissance;  // Base masks (can be modified by genotype)
    uint16_t masque_conditions_survie;     // Base masks
    CelluleEvolutive *grille_cellules_actuelles;     // Grid of cells with properties
    CelluleEvolutive *grille_cellules_suivantes;     // Temporary grid for calculations
    EnvironnementLocal *grille_environnement;        // Environment of each cell
    uint32_t generation_actuelle;                     // Generation counter
    uint32_t population_totale;                       // Number of living cells
} AutomateCellulaire;

// Analyzes the rule string and fills the condition masks
// Example: "B3/S23" means birth with 3 neighbors, survival with 2 or 3 neighbors
void analyser_regles_automate(AutomateCellulaire *automate);

// =============================
// MODULAR INITIALIZATION FUNCTIONS
// =============================

// Initializes the grid according to chosen type (see TypeInitialisation)
void initialiser_grille_selon_type(AutomateCellulaire *automate, TypeInitialisation type, uint32_t graine_aleatoire);

// Specialized initialization functions (can be used directly)
void initialiser_grille_uniforme(AutomateCellulaire *automate, uint32_t graine_aleatoire);
void initialiser_grille_centre(AutomateCellulaire *automate, uint32_t graine_aleatoire);
void initialiser_grille_clusters(AutomateCellulaire *automate, uint32_t graine_aleatoire);

// Compatibility function (old interface)
void initialiser_grille_aleatoire(AutomateCellulaire *automate, uint32_t graine_aleatoire);


// Calculates a new generation by applying automaton rules
// This is the heart of the simulation: counts neighbors and applies rules
void calculer_generation_suivante(AutomateCellulaire *automate);

// Displays the current grid in VGA memory (text mode)
// Uses race characters for living cells and ' ' for dead cells
void afficher_grille_sur_ecran(const AutomateCellulaire *automate, volatile uint8_t *memoire_vga);

#endif // CA_H
