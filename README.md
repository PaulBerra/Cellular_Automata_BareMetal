#  Cellular Automaton Kernel - Advanced Biological Evolution Simulator

A bare-metal kernel implementing a realistic evolutionary cellular automaton with advanced biological mechanisms including predation, epidemics, environmental cycles, and adaptive mutations.

##  Quick Start

### Compilation
```bash
make clean && make
```

### Running
```bash
# Standard window
qemu-system-i386 -cdrom CellularAutomatKerna.iso

# Fullscreen mode (press Ctrl+Alt+F to toggle)
qemu-system-i386 -cdrom CellularAutomatKerna.iso -full-screen

# Larger window
qemu-system-i386 -cdrom CellularAutomatKerna.iso -vga std -display sdl,window-close=off
```

### Display
- **Screen**: 80×25 fullscreen VGA text mode
- **Characters**: `E` (Explorer), `C` (Colonizer), `N` (Nomad), `A` (Adaptive)
- **Colors**: Blue→Cyan→Green→Yellow→Red (age progression)
- **Info**: Bottom-right shows `Gen:##### P:###` (generation and population)
- **Fullscreen**: Use `-full-screen` flag or Ctrl+Alt+F to toggle

##  Configuration Parameters

### Core Evolution Settings (`src/ca.h`)

```c
#define AGE_MAXIMUM 1200                    // Maximum cell lifespan
#define FERTILITE_DEBUT 5                   // Minimum reproduction age
#define FERTILITE_OPTIMALE 30               // Peak fertility age
#define TAUX_MUTATION 8                     // Base mutation rate (%)
```

### Biological Realism Cycles
```c
#define CYCLES_ENVIRONNEMENTAUX 150         // Environmental cycle period
#define PREDATION_CYCLE 80                  // Predator-prey cycle
#define EPIDEMIC_CYCLE 120                  // Disease outbreak cycle
#define FOOD_SCARCITY_CYCLE 90              // Resource scarcity cycle
```

### Environmental Pressures
```c
#define PREDATION_PRESSURE 15               // Predation mortality (%)
#define EPIDEMIC_MORTALITY 20               // Disease mortality (%)
#define BASE_MUTATION_RATE 3                // Normal mutation rate (%)
#define STRESS_MUTATION_MULTIPLIER 4        // Stress-induced mutation boost
```

### Grid Density
```c
#define DENSITE_MINIMUM 15                  // Min initial density (%)
#define DENSITE_MAXIMUM 80                  // Max initial density (%)
#define BONUS_CLUSTERING 6                  // Clustering bonus (%)
```

### Simulation Speed
```c
#define VITESSE_SIMULATION 5000000          // Delay between generations (higher = slower)
```

**Recommended values:**
- `1000000` - Very fast
- `5000000` - Normal speed (default)
- `10000000` - Slow
- `20000000` - Very slow for detailed observation

##  Cellular Automaton Rules

### Rule Format
Rules follow the standard **"B/S" notation**:
- **B**: Birth conditions (number of neighbors required for birth)
- **S**: Survival conditions (number of neighbors required for survival)

### Current Rules
```c
#define REGLES_AUTOMATE "B36/S23"           // HighLife rules
```

### Alternative Rules
```c
// Conway's Game of Life (classic but tends to stabilize)
#define REGLES_AUTOMATE "B3/S23"

// Seeds (chaotic, explosive growth)
#define REGLES_AUTOMATE "B2/S23"

// 34 Life (different stable structures)
#define REGLES_AUTOMATE "B34/S34"

// Replicator (self-copying patterns)
#define REGLES_AUTOMATE "B1357/S1357"
```

### Rule Examples
- **B3/S23**: Cell born with 3 neighbors, survives with 2-3 neighbors
- **B36/S23**: Cell born with 3 or 6 neighbors, survives with 2-3 neighbors
- **B2/S23**: Cell born with 2 neighbors, survives with 2-3 neighbors

##  Random Grid Generation

### Generation Types (`src/kernel.c`)

The system supports three initialization patterns:

#### 1. Uniform Distribution
```c
initialiser_grille_uniforme(&automate, seed);
```
- Even distribution across entire grid
- Uses average of min/max density
- Best for testing basic rules

#### 2. Center-Focused Distribution
```c
initialiser_grille_centre(&automate, seed);
```
- Higher density at center, lower at edges
- Creates natural population gradients
- Simulates initial colonization patterns

#### 3. Clustered Distribution (Default)
```c
initialiser_grille_clusters(&automate, seed);
```
- Natural clustering with neighbor bonus
- Realistic biological distributions
- Includes genetic diversity initialization

### Using the Modular Interface
```c
// More explicit control
initialiser_grille_selon_type(&automate, INIT_ALEATOIRE_CLUSTERS, seed);
initialiser_grille_selon_type(&automate, INIT_ALEATOIRE_CENTRE, seed);
initialiser_grille_selon_type(&automate, INIT_ALEATOIRE_UNIFORME, seed);
```

### Random Seed Control
```c
// In src/kernel.c, line 47:
initialiser_grille_aleatoire(&mon_automate, 0x94215687);
```
- Change the hex value for different starting patterns
- Each seed produces reproducible initial conditions
- Use system time or other entropy sources for true randomness

### Generation Algorithm
The clustering algorithm works as follows:
1. **Base density**: Uses configured min/max percentages
2. **Spatial gradient**: Higher density toward center
3. **Neighbor bonus**: +6% probability near existing cells
4. **Genetic initialization**: Random traits for each cell
5. **Species assignment**: Based on spatial location

##  Biological Evolution Features

### Multi-Trait Evolution
Each cell evolves multiple biological traits:
- **Disease Resistance**: Survival during epidemics
- **Predation Camouflage**: Avoiding predators
- **Energy Efficiency**: Resource utilization
- **Territorial Behavior**: Competition dynamics
- **Stress Adaptation**: Mutation rate response

### Environmental Pressures
- **Predation Gradients**: Higher pressure at grid edges
- **Epidemic Cycles**: Disease spread based on density
- **Resource Scarcity**: Seasonal food availability
- **Territorial Competition**: Density-dependent stress

### Anti-Stagnation Mechanisms
1. **Cyclical Selection**: Different traits favored over time
2. **Stress-Adaptive Mutations**: Higher mutation under pressure
3. **Spatial Niches**: 4 ecological zones with different advantages
4. **Evolutionary Arms Race**: Continuous adaptation pressure

##  Advanced Customization

### Creating New Rules
To add custom automaton rules:
1. Modify `REGLES_AUTOMATE` in `src/ca.h`
2. Use format "B[birth_neighbors]/S[survival_neighbors]"
3. Example: "B238/S23" = birth with 2,3,8 neighbors, survive with 2,3

### Adjusting Evolution Speed
- Increase `TAUX_MUTATION` for faster evolution
- Decrease cycle periods for rapid environmental changes
- Adjust `STRESS_MUTATION_MULTIPLIER` for adaptation intensity

### Environmental Tuning
- Modify pressure constants for different ecosystem dynamics
- Adjust cycle periods to match desired simulation timescales
- Balance mortality rates to prevent extinction/overpopulation

##  Understanding the Display

### Cell Representation
- **Character**: Indicates cell race/species
- **Color**: Shows cell age (blue=young, red=old)
- **Position**: Spatial location affects survival chances

### Information Panel
- **Gen:####**: Current generation number (up to 99,999)
- **P:###**: Current population count

### Evolution Indicators
Watch for:
- Population cycles during environmental stress
- Color shifts indicating age structure changes
- Character distribution showing species adaptation
- Spatial patterns revealing migration and territoriality

##  Troubleshooting

### No Evolution/Stagnation
- Check if all cells died (increase initial density)
- Verify environmental cycles are active
- Ensure mutation rates are sufficient
- Try different initial seeds

### Population Explosion
- Reduce initial density parameters
- Increase environmental pressure
- Adjust predation/epidemic mortality rates

### Compilation Issues
- Ensure GCC with 32-bit support is installed
- Check that all source files are present
- Verify QEMU is available for testing

##  References

This implementation is based on:
- Conway's Game of Life and variants
- Population biology and evolutionary theory
- Epidemiological modeling principles
- Predator-prey dynamics
- Stress-response evolution mechanisms

---

**Note**: This is a bare-metal kernel simulation. It boots directly and requires no operating system. The biological mechanisms are mathematically modeled approximations of real evolutionary processes.
