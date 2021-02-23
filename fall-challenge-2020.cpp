#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <ctime> 
#include <cmath>

using namespace std;

#define DEBUG 0
#define DEBUG_evaluatePlayingAction 1
#define DEBUG_evalReachCommandScore 0
#define DEBUG_evalLearnScore 0
#define DEBUG_1 0

typedef struct inventory_t{
    vector<int> inv;
    int score; // amount of rupees
}inventory_t;

typedef struct action_t{
    int actionId; // the unique ID of this spell or recipe
    string actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
    int delta[4];
    int price; // the price in rupees if this is a potion
    int tomeIndex; // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax; For brews, this is the value of the current urgency bonus
    int taxCount; // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell; For brews, this is how many times you can still gain an urgency bonus
    bool castable; // in the first league: always 0; later: 1 if this is a castable player spell
    bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell
}action_t;

typedef struct evaluationData_t{
    vector<action_t*> commands;
    vector<vector<vector<int>>> commandsInvDeltaDiffs;
    bool valid;
    // For learn action only
    vector<int> sumDeltaSpells;
    vector<int> sumDeltaSpellsAbs;
}evaluationData_t;

typedef struct scoreParameters_t{
    float score_learn; // meanSumDeltaSpellsIngredients + meanSumDeltaSpells + meanSumDeltaSpellsAbs
    float score_reachCommand; // Price / distanceToCommand
}scoreParameters_t;

typedef struct playingAction_t{
    action_t action;
    evaluationData_t evaluationData;
    scoreParameters_t scoreParameters;
    float score; // score_learn + score_reachCommand
    float cast_repeatitionIndex;
}playingAction_t;


vector<playingAction_t> generatePlayingActions(vector<action_t>* availablePlayingActions);
void evalReachCommandScore(playingAction_t* playingAction, inventory_t inventory, vector<action_t>* availableCommands);
void evalLearnScore(playingAction_t* playingAction, vector<playingAction_t>* availableCasts);
void evaluatePlayingAction(playingAction_t* playingAction);

void logAction(string actionName, action_t* action);
void logInventory(string inventoryName, inventory_t* inventory);
action_t* bestCanBrew(inventory_t* inventory, vector<action_t>* commands);

int main()
{
    vector<action_t> availablePlayingActions;
    vector<action_t> availableCommands;
    inventory_t myInventory;
    myInventory.inv = *(new vector<int>(4));
    inventory_t sisterInventory;
    sisterInventory.inv = *(new vector<int>(4));
    // game loop
    while (1) {
        availablePlayingActions.clear();
        availableCommands.clear();
        int actionCount; // the number of spells and recipes in play
        cin >> actionCount; cin.ignore();
        for (int i_action = 0; i_action < actionCount; i_action++) {
            static action_t action = {0};
            cin >> action.actionId >> action.actionType >> action.delta[0] >> action.delta[1] >> action.delta[2] >> action.delta[3] >> action.price >> action.tomeIndex >> action.taxCount >> action.castable >> action.repeatable; cin.ignore();
            if (action.actionType == "CAST"){
                bool actionInList = false;
                for (int i = 0; i<availablePlayingActions.size(); i++){
                    if (action.actionId == availablePlayingActions[i].actionId){
                        availablePlayingActions[i] = action;
                        actionInList = true;
                        break;
                    }
                }
                if (!actionInList){
                    availablePlayingActions.push_back(action);
                }
            }            
            else if (action.actionType == "LEARN"){
                bool actionInList = false;
                for (int i = 0; i<availablePlayingActions.size(); i++){
                    if (action.actionId == availablePlayingActions[i].actionId){
                        availablePlayingActions[i] = action;
                        actionInList = true;
                        break;
                    }
                }
                if (!actionInList){
                    availablePlayingActions.push_back(action);
                }
            }
            else if (action.actionType == "BREW"){
                bool actionInList = false;
                for (int i = 0; i<availableCommands.size(); i++){
                    if (action.actionId == availableCommands[i].actionId){
                        availableCommands[i] = action;
                        actionInList = true;
                        break;
                    }
                }
                if (!actionInList){
                    availableCommands.push_back(action);
                }
            }
        }
        if (DEBUG_1){
            cerr << "availablePlayingActions" << endl;
            for (int i = 0; i<availablePlayingActions.size(); i++){
                string name = "availablePlayingActions[" + to_string(i) + "]";
                logAction(name, &availablePlayingActions[i]);
            }
            cerr << "availableCommands" << endl;
            for (int i = 0; i<availableCommands.size(); i++){
                string name = "availableCommands[" + to_string(i) + "]";
                logAction(name, &availableCommands[i]);
            }
        }
        cin >> myInventory.inv[0] >> myInventory.inv[1] >> myInventory.inv[2] >> myInventory.inv[3] >> myInventory.score; cin.ignore();

        cin >> sisterInventory.inv[0] >> sisterInventory.inv[1] >> sisterInventory.inv[2] >> sisterInventory.inv[3] >> sisterInventory.score; cin.ignore();
        
        if (DEBUG_1){
            cerr << "myInventory" << endl;
            logInventory("myInventory", &myInventory);
            cerr << "myInventory" << endl;
            logInventory("sisterInventory", &sisterInventory);
        }
        action_t* bestCommandCanBrew = bestCanBrew(&myInventory, &availableCommands);
        if (bestCommandCanBrew != NULL){
            cout << "BREW" << " " << bestCommandCanBrew->actionId << endl;
        }
        else{
            vector<playingAction_t> playingActions = generatePlayingActions(&availablePlayingActions);
            playingAction_t* bestPlayingAction = NULL;
            float bestScore = -1;
            for (int i_playingAction=0; i_playingAction<playingActions.size(); i_playingAction++){
                evalReachCommandScore(&playingActions[i_playingAction], myInventory, &availableCommands);
                //evalLearnScore(&playingActions[i_playingAction], &playingActions);
                evaluatePlayingAction(&playingActions[i_playingAction]);
                if (playingActions[i_playingAction].score > bestScore && playingActions[i_playingAction].evaluationData.valid){
                    bestScore = playingActions[i_playingAction].score;
                    bestPlayingAction = &playingActions[i_playingAction];
                }
            }
            if (bestPlayingAction->action.actionType == "LEARN"){
                cout << "LEARN" << " " << bestPlayingAction->action.actionId << endl;
            }
            else if (bestPlayingAction->action.actionType == "CAST"){
                if (bestPlayingAction->action.castable){
                cout << "CAST" << " " << bestPlayingAction->action.actionId << " times " << bestPlayingAction->cast_repeatitionIndex << endl;
                }
                else{
                    cout << "REST" << endl;
                }
            }
        }

        // in the first league: BREW <id> | WAIT; later: BREW <id> | CAST <id> [<times>] | LEARN <id> | REST | WAIT
        //cout << "WAIT" << endl;
    }
}

action_t* bestCanBrew(inventory_t* inventory, vector<action_t>* commands){
    action_t* bestCanBrew = NULL;
    int bestScore = -1;
    for(int i=0; i<commands->size(); i++){
        bool canBrew=true;
        for (int j=0; j<4; j++){
            int diff = inventory->inv[j] + commands->at(i).delta[j];
            if (diff < 0){
                canBrew = false;
            }
        }
        if (canBrew && bestScore < commands->at(i).price){
            bestScore = commands->at(i).price;
            bestCanBrew = &commands->at(i);
        }
    }
    return bestCanBrew;
}

vector<playingAction_t> generatePlayingActions(vector<action_t>* availablePlayingActions){
    vector<playingAction_t> playingActions(availablePlayingActions->size());
    for (int i=0; i<availablePlayingActions->size(); i++){
        playingAction_t playingAction = {-1};
        playingAction.action = availablePlayingActions->at(i);
        playingAction.score = 0;
        playingActions[i] = playingAction;
    }
    return playingActions;
}
    action_t* command;
    int deltaInv[4];

void evalReachCommandScore(playingAction_t* playingAction, inventory_t inventory, vector<action_t>* availableCommands){
    // Calcule le score lié à la difference entre le delta du cast ou learn appliqué à chaque commande.
    // Prend en compte repeatable et le cout d'une learnAction
    float weight_commandPrice = 1;
    float weight_distanceToCommand = 1; // La différence entre un sort appliqué à l'inventaire et la commande min(mean(commandsInVDeltaDiffs[i_command]))
    evaluationData_t* evaluationData = &playingAction->evaluationData;
    // TODO: add taxeCount
    if (playingAction->action.actionType == "LEARN"){
        bool learnActionValid = !(inventory.inv[0] - playingAction->action.tomeIndex < 0);
        playingAction->evaluationData.valid = learnActionValid;
        if (!learnActionValid){
            playingAction->scoreParameters.score_reachCommand = 0;
            cerr << "LEARN " << playingAction->action.actionId << " not enough inv[0]" << endl;
            return;
        }
    }
    float max_scoreReachCommand = -99999;
    float repeatitionIndex = -1;
    bool castActionValid = true;
    int debug_i_command;
    for (int i_command=0; i_command<availableCommands->size(); i_command++){
        vector<int> invDeltaDiff = inventory.inv;
        if (playingAction->action.actionType == "LEARN"){
            invDeltaDiff[0] -= playingAction->action.tomeIndex;
        }
        bool repeatable = playingAction->action.repeatable;
        bool outOfStock = false;
        int n_repetition = 0;
        float distToCmommand = 0;
        float min_distToCommand = 99999;
        int best_repeatitionIndex = 0; 
        while (!outOfStock && (n_repetition<1 || repeatable)){
            for (int j=0; j<4; j++){
                invDeltaDiff[j] += playingAction->action.delta[j];
                distToCmommand += abs(invDeltaDiff[j] + availableCommands->at(i_command).delta[j]);
                if (invDeltaDiff[j] < 0){
                    outOfStock = true;
                    if(n_repetition==0 && playingAction->action.actionType == "CAST"){
                        castActionValid = false;
                    }
                }
            }
            if (!outOfStock){
                distToCmommand /= 4.;
                if (min_distToCommand > distToCmommand){
                    min_distToCommand = distToCmommand;
                    best_repeatitionIndex = n_repetition + 1;
                }
            }
            n_repetition ++;
        }
        float commandPrice = availableCommands->at(i_command).price + availableCommands->at(i_command).tomeIndex;
        float score_distanceToCommand = min_distToCommand * weight_distanceToCommand;
        float score_commandPrice = commandPrice * weight_commandPrice;
        float score_reachCommand = score_commandPrice/score_distanceToCommand;
        if (score_reachCommand > max_scoreReachCommand){
            max_scoreReachCommand = score_reachCommand;
            repeatitionIndex = best_repeatitionIndex;
            debug_i_command = i_command;
        }
    }
    playingAction->scoreParameters.score_reachCommand = max_scoreReachCommand;
    if (playingAction->action.actionType == "CAST"){
        playingAction->cast_repeatitionIndex = repeatitionIndex;
        playingAction->evaluationData.valid = castActionValid;
    }

    if (DEBUG_evalReachCommandScore){
        cerr << playingAction->action.actionType  << " " << playingAction->action.actionId << " valid " << playingAction->evaluationData.valid << endl;
        cerr << "score_reachCommand " << playingAction->scoreParameters.score_reachCommand << " cmd " << availableCommands->at(debug_i_command).actionId << endl;
        cerr << "repeatitionIndex " << playingAction->cast_repeatitionIndex << endl;
        cerr << endl;
    }

}

void evalLearnScore(playingAction_t* playingAction, vector<playingAction_t>* availableCasts){
    
    float weight_deltaSpellsIngredients = 1; // L'ecart moyen entre les ingredients de la somme des sorts sumDeltaSpells. Doit tendre vers 0
    float weight_meanSumDeltaSpells = 1; // La moyenne sur les ingredients de la somme des sorts sumDeltaSpells. Doit tendre vers 0
    float weight_meanSumDeltaSpellsAbs = 1; // La moyenne sur les ingredients de la somme des sorts absolu. Doit tendre vers +

    int n_cast = 0;
    vector<int> sumDeltaSpells(4, 0);
    vector<int> sumDeltaSpellsAbs(4, 0);
    // compute for availableCasts
    for (int i_cast=0; i_cast<availableCasts->size(); i_cast++){
        action_t* cast = &availableCasts->at(i_cast).action;
        if (cast->actionType == "CAST"){
            for (int j=0; j<4; j++){
                sumDeltaSpells[j] += cast->delta[j];
                sumDeltaSpellsAbs[j] += abs(cast->delta[j]);
            }
        }
    n_cast ++;
    }
    // On ajouter un score_learn à chaque playingAction le qui correspond au score moyen des availableCasts pour "normaliser" entre les learnAction et castActions
    if (playingAction->action.actionType != "LEARN"){
        float meanSumDeltaSpells;
        float meanSumDeltaSpellsAbs;
        float min_sumDeltaSpells = 99999;
        float max_sumDeltaSpells = -1;
        for (int j=0; j<4; j++){
            float meanSumDeltaSpells = sumDeltaSpells[j]/(float)n_cast;
            float meanSumDeltaSpellsAbs = sumDeltaSpellsAbs[j]/(float)n_cast;
            if (min_sumDeltaSpells > meanSumDeltaSpells)
                min_sumDeltaSpells = meanSumDeltaSpells;
            if (max_sumDeltaSpells < meanSumDeltaSpells)
                max_sumDeltaSpells = meanSumDeltaSpells;
        }
        float deltaSpellsIngredients = max_sumDeltaSpells - min_sumDeltaSpells;
        float score_deltaSpell = meanSumDeltaSpells * weight_meanSumDeltaSpells;
        float score_deltaSpellsIngredients = deltaSpellsIngredients * weight_deltaSpellsIngredients;
        float score_deltaSpellAbs = meanSumDeltaSpellsAbs * weight_meanSumDeltaSpellsAbs;
        float score_learn = score_deltaSpellAbs/(n_cast*(score_deltaSpell + score_deltaSpellsIngredients));
        playingAction->scoreParameters.score_learn = score_learn;
    }

    // On ajoute la difference entre les moyennes des availableCasts et le delta du sort learn pour les learnAction,     
    if (playingAction->action.actionType == "LEARN"){
        float meanSumDeltaSpellsPlusLearn;
        float meanSumDeltaSpellsAbsPlusLearn;
        float min_sumDeltaSpells = 99999;
        float max_sumDeltaSpells = -1;
        for (int j=0; j<4; j++){
            float meanSumDeltaSpells = sumDeltaSpells[j]/(float)n_cast;
            float meanSumDeltaSpellsAbs = sumDeltaSpellsAbs[j]/(float)n_cast;
            meanSumDeltaSpellsPlusLearn = meanSumDeltaSpells + playingAction->action.delta[j];
            meanSumDeltaSpellsAbsPlusLearn = meanSumDeltaSpellsAbs + abs(playingAction->action.delta[j]);
            if (min_sumDeltaSpells > meanSumDeltaSpellsPlusLearn)
                min_sumDeltaSpells = meanSumDeltaSpellsPlusLearn;
            if (max_sumDeltaSpells < meanSumDeltaSpellsPlusLearn)
                max_sumDeltaSpells = meanSumDeltaSpellsPlusLearn;
        }

        float deltaSpellsIngredients = max_sumDeltaSpells - min_sumDeltaSpells;
        float score_deltaSpell = meanSumDeltaSpellsPlusLearn * weight_meanSumDeltaSpells;
        float score_deltaSpellsIngredients = deltaSpellsIngredients * weight_deltaSpellsIngredients;
        float score_deltaSpellAbs = meanSumDeltaSpellsAbsPlusLearn * weight_meanSumDeltaSpellsAbs;
        float score_learn = score_deltaSpellAbs/(score_deltaSpell + score_deltaSpellsIngredients);
        playingAction->scoreParameters.score_learn = score_learn;
    }
    if (DEBUG_evalLearnScore){
        cerr << playingAction->action.actionType  << " " << playingAction->action.actionId << " valid " << playingAction->evaluationData.valid << endl;
        cerr << "score_learn " << playingAction->scoreParameters.score_learn << endl;
        cerr << endl;
    }
}


void evaluatePlayingAction(playingAction_t* playingAction){
    float weight_scoreReachCommand = 1.;
    float weight_scoreLearn = 1.;
    float score_reachCommand = playingAction->scoreParameters.score_reachCommand;
    float score_learn = playingAction->scoreParameters.score_learn;
    playingAction->score = score_reachCommand * weight_scoreReachCommand + score_learn * weight_scoreLearn;
    if (DEBUG_evaluatePlayingAction){
        cerr << playingAction->action.actionType  << " " << playingAction->action.actionId << " valid " << playingAction->evaluationData.valid << endl;
        cerr << "score_reachCommand " << playingAction->scoreParameters.score_reachCommand << " repeatitionIndex " << playingAction->cast_repeatitionIndex << endl;
        cerr << "score_learn " << playingAction->scoreParameters.score_learn << endl;
        cerr << "score_total " << playingAction->score << endl;
        cerr << endl;
    }

}

//void computeReachCommandScore(int* bestRepetitionIndex, )

void logAction(string actionName, action_t* action){
    cerr << actionName << "->actionId = " << action->actionId << endl;
    cerr << actionName << "->actionType = " << action->actionType << endl;
    cerr << actionName << "->delta0 = " << action->delta[0] << endl;
    cerr << actionName << "->delta1 = " << action->delta[1] << endl;
    cerr << actionName << "->delta2 = " << action->delta[2] << endl;
    cerr << actionName << "->delta3 = " << action->delta[3] << endl;
    cerr << actionName << "->price = " << action->price << endl;
    cerr << actionName << "->tomeIndex = " << action->tomeIndex << endl;
    cerr << actionName << "->taxCount = " << action->taxCount << endl;
    cerr << actionName << "->castable = " << action->castable << endl;
    cerr << actionName << "->repeatable = " << action->repeatable << endl;
}

void logInventory(string inventoryName, inventory_t* inventory){
    cerr << inventoryName << "->inv0 = " << inventory->inv[0] << endl;
    cerr << inventoryName << "->inv1 = " << inventory->inv[1] << endl;
    cerr << inventoryName << "->inv2 = " << inventory->inv[2] << endl;
    cerr << inventoryName << "->inv3 = " << inventory->inv[3] << endl;
    cerr << inventoryName << "->score = " << inventory->score << endl;
}
