#include "ScoreContext.h"
#include "AggregateNotesFilter.h"
#include "IO.h"
#include "Utilities.h"
#include "UI.h"
#include <vector>

using json = nlohmann::json;
using namespace IO;

namespace MikuMikuWorld
{
	constexpr const char* clipboardSignature = "MikuMikuWorld clipboard\n";

	static InverseNotesFilter inverseGuideFilter(CommonNoteFilters::guideFilter());

	static bool noteExists(const int id, const Score& score)
	{
		return score.notes.find(id) != score.notes.end();
	}

	void ScoreContext::setStep(HoldStepType type)
	{
		AggregateNotesFilter filterBuilder;
		const NoteSelection filteredNotes = filterBuilder
			.add(CommonNoteFilters::stepFilter())
			.add(&inverseGuideFilter)
			.filter(selectedNotes, score);
		
		if (filteredNotes.empty())
			return;

		bool edit = false;
		const Score prev = score;
		
		for (int id : filteredNotes)
		{
			HoldNote& hold = score.holdNotes.at(score.notes.at(id).parentID);

			// The filter already verifies the step belongs to the hold
			const int pos = findHoldStep(hold, id);
			if (type == HoldStepType::HoldStepTypeCount)
			{
				cycleStepType(hold.steps[pos]);
				edit = true;
			}
			else
			{
				edit |= hold.steps[pos].type != type;
				hold.steps[pos].type = type;
			}
		}

		if (edit)
			pushHistory("Change step type", prev, score);
	}

	void ScoreContext::setFlick(FlickType flick)
	{
		const NoteSelection filteredNotes = CommonNoteFilters::flickableFilter()->filter(selectedNotes, score);
		if (filteredNotes.empty())
			return;

		bool edit = false;
		const Score prev = score;

		if (flick == FlickType::FlickTypeCount)
		{
			edit = true;
			for (int id : filteredNotes)
				cycleFlick(score.notes.at(id));
		}
		else
		{
			for (int id : filteredNotes)
			{
				Note& note = score.notes.at(id);
				edit |= note.flick != flick;
				note.flick = flick;
			}
		}

		if (edit)
			pushHistory("Change flick", prev, score);
	}

	void ScoreContext::setEase(EaseType ease)
	{
		const NoteSelection filteredNotes = CommonNoteFilters::easeFilter()->filter(selectedNotes, score);
		if (filteredNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : filteredNotes)
		{
			Note& note = score.notes.at(id);
			HoldNote& hold = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
			HoldStep& step = note.getType() == NoteType::Hold ? hold.start : hold.steps[findHoldStep(hold, note.ID)];

			if (ease == EaseType::EaseTypeCount)
			{
				cycleStepEase(step);
				edit = true;
			}
			else
			{
				edit |= step.ease != ease;
				step.ease = ease;
			}
		}

		if (edit)
			pushHistory("Change ease", prev, score);
	}

	void ScoreContext::setHoldType(HoldNoteType hold)
	{
		if (selectedNotes.empty())
			return;

		bool edit = false;
		Score prev = score;
		for (int id : selectedNotes)
		{
			if (!noteExists(id, score))
				continue;
			
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				HoldNote& holdNote = score.holdNotes.at(id);
				edit |= holdNote.startType != hold;
				holdNote.startType = hold;
			}
			else if (note.getType() == NoteType::HoldEnd)
			{
				HoldNote& holdNote = score.holdNotes.at(note.parentID);
				edit |= holdNote.endType != hold;
				holdNote.endType = hold;
			}
		}

		pushHistory("Change hold", prev, score);
	}

	void ScoreContext::toggleCriticals()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		std::unordered_set<int> critHolds;
		for (int id : selectedNotes)
		{
			if (!noteExists(id, score))
				continue;
			
			if (Note& note = score.notes.at(id); note.getType() == NoteType::Tap)
			{
				note.critical ^= true;
			}
			else if (note.getType() == NoteType::HoldEnd && (note.isFlick() || note.friction))
			{
				// if the start is critical the entire hold must be critical
				note.critical = score.notes.at(note.parentID).critical ? true : !note.critical;
			}
			else
			{
				critHolds.insert(note.getType() == NoteType::Hold ? note.ID : note.parentID);
			}
		}

		for (auto& hold : critHolds)
		{
			// flip critical state
			HoldNote& note = score.holdNotes.at(hold);
			bool critical = !score.notes.at(note.start.ID).critical;

			// again if the hold start is critical, every note in the hold must be critical
			score.notes.at(note.start.ID).critical = critical;
			score.notes.at(note.end).critical = critical;
			for (auto& step : note.steps)
				score.notes.at(step.ID).critical = critical;
		}

		pushHistory("Change critical note", prev, score);
	}

	void ScoreContext::toggleFriction()
	{
		const NoteSelection filteredNotes = CommonNoteFilters::frictionableFilter()->filter(selectedNotes, score);
		if (filteredNotes.empty())
			return;

		Score prev = score;
		for (int id : filteredNotes)
		{
			Note& note = score.notes.at(id);
			if (note.getType() == NoteType::Hold)
			{
				score.holdNotes.at(id).startType = HoldNoteType::Normal;
			}
			else if (note.getType() == NoteType::HoldEnd)
			{
				score.holdNotes.at(note.parentID).endType = HoldNoteType::Normal;
				if (!note.isFlick() && note.friction && !score.notes.at(note.parentID).critical)
				{
					// Prevent critical hold end if the hold start is not critical
					note.critical = false;
				}
			}
			
			note.friction = !note.friction;
		}

		pushHistory("Change trace notes", prev, score);
	}

	void ScoreContext::deleteSelection()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		for (auto& id : selectedNotes)
		{
			if (!noteExists(id, score))
				continue;

			if (Note& note = score.notes.at(id); note.getType() != NoteType::Hold && note.getType() != NoteType::HoldEnd)
			{
				if (note.getType() == NoteType::HoldMid)
				{
					// find hold step and remove it from the steps data container
					if (score.holdNotes.find(note.parentID) != score.holdNotes.end())
					{
						std::vector<HoldStep>& steps = score.holdNotes.at(note.parentID).steps;
						steps.erase(std::find_if(steps.cbegin(), steps.cend(), [id](const HoldStep& s)
							{ return s.ID == id; }));
					}
				}
				score.notes.erase(id);
			}
			else
			{
				const HoldNote& hold = score.holdNotes.at(note.getType() == NoteType::Hold ? note.ID : note.parentID);
				score.notes.erase(hold.start.ID);
				score.notes.erase(hold.end);

				// hold steps cannot exist without a hold
				for (const auto& step : hold.steps)
					score.notes.erase(step.ID);

				score.holdNotes.erase(hold.start.ID);
			}
		}

		selectedNotes.clear();
		pushHistory("Delete notes", prev, score);
	}

	void ScoreContext::flipSelection()
	{
		if (selectedNotes.empty())
			return;

		Score prev = score;
		for (int id : selectedNotes)
		{
			if (!noteExists(id, score))
				continue;
			
			Note& note = score.notes.at(id);
			note.lane = MAX_LANE - note.lane - note.width + 1;

			if (note.flick == FlickType::Left)
				note.flick = FlickType::Right;
			else if (note.flick == FlickType::Right)
				note.flick = FlickType::Left;
		}

		pushHistory("Flip notes", prev, score);
	}

	void ScoreContext::cutSelection()
	{
		copySelection();
		deleteSelection();
	}

	void ScoreContext::copySelection()
	{
		if (selectedNotes.empty())
			return;
		
		std::string clipboard{ clipboardSignature };
		clipboard.append(jsonIO::noteSelectionToJson(score, selectedNotes, minTickFromSelection()).dump());

		ImGui::SetClipboardText(clipboard.c_str());
	}

	void ScoreContext::cancelPaste()
	{
		pasteData.pasting = false;
	}

	void ScoreContext::doPasteData(const json& data, bool flip)
	{
		int baseId = 0;
		pasteData.notes.clear();
		pasteData.holds.clear();

		if (jsonIO::arrayHasData(data, "notes"))
		{
			for (const auto& entry : data["notes"])
			{
				Note note = jsonIO::jsonToNote(entry, NoteType::Tap);
				note.ID = baseId++;

				pasteData.notes[note.ID] = note;
			}
		}

		if (jsonIO::arrayHasData(data, "holds"))
		{
			for (const auto& entry : data["holds"])
			{
				if (!jsonIO::keyExists(entry, "start") || !jsonIO::keyExists(entry, "end"))
					continue;

				Note start = jsonIO::jsonToNote(entry["start"], NoteType::Hold);
				start.ID = baseId++;
				pasteData.notes[start.ID] = start;

				Note end = jsonIO::jsonToNote(entry["end"], NoteType::HoldEnd);
				end.ID = baseId++;
				end.parentID = start.ID;
				end.critical = start.critical || ((end.isFlick() || end.friction) && end.critical);
				pasteData.notes[end.ID] = end;

				std::string startEase = jsonIO::tryGetValue<std::string>(entry["start"], "ease", "linear");
				int startEaseTypeIndex = findArrayItem(startEase.c_str(), easeTypes, arrayLength(easeTypes));
				if (startEaseTypeIndex == -1)
				{
					startEaseTypeIndex = 0;
					if (startEase == "in") startEaseTypeIndex = 1;
					if (startEase == "out") startEaseTypeIndex = 2;
				}

				HoldNote hold{ { start.ID, HoldStepType::Normal, (EaseType)startEaseTypeIndex }, {}, end.ID };
				if (jsonIO::keyExists(entry, "steps"))
				{
					hold.steps.reserve(entry["steps"].size());
					for (const auto& step : entry["steps"])
					{
						Note mid = jsonIO::jsonToNote(step, NoteType::HoldMid);
						mid.critical = start.critical;
						mid.ID = baseId++;
						mid.parentID = start.ID;
						pasteData.notes[mid.ID] = mid;

						std::string midType = jsonIO::tryGetValue<std::string>(step, "type", "normal");
						std::string midEase = jsonIO::tryGetValue<std::string>(step, "ease", "linear");
						int stepTypeIndex = findArrayItem(midType.c_str(), stepTypes, arrayLength(stepTypes));
						int easeTypeIndex = findArrayItem(midEase.c_str(), easeTypes, arrayLength(stepTypes));

						// Maintain compatibility with old step type names
						if (stepTypeIndex == -1)
						{
							stepTypeIndex = 0;
							if (midType == "invisible") stepTypeIndex = 1;
							if (midType == "ignored") stepTypeIndex = 2;
						}

						// Maintain compatibility with old ease type names
						if (easeTypeIndex == -1)
						{
							easeTypeIndex = 0;
							if (midEase == "in") easeTypeIndex = 1;
							if (midEase == "out") easeTypeIndex = 2;
						}

						hold.steps.push_back({ mid.ID, (HoldStepType)stepTypeIndex, (EaseType)easeTypeIndex });
					}
				}

				std::string startType = jsonIO::tryGetValue<std::string>(entry["start"], "type", "normal");
				std::string endType = jsonIO::tryGetValue<std::string>(entry["end"], "type", "normal");

				if (startType == "guide" || endType == "guide")
				{
					hold.startType = hold.endType = HoldNoteType::Guide;
					start.friction = end.friction = false;
					end.flick = FlickType::None;
				}
				else
				{
					if (startType == "hidden")
					{
						hold.startType = HoldNoteType::Hidden;
						start.friction = false;
					}

					if (endType == "hidden")
					{
						hold.endType = HoldNoteType::Hidden;
						end.friction = false;
						end.flick = FlickType::None;
					}
				}

				pasteData.holds[hold.start.ID] = hold;
			}
		}

		if (flip)
		{
			for (auto& [_, note] : pasteData.notes)
			{
				note.lane = MAX_LANE - note.lane - note.width + 1;

				if (note.flick == FlickType::Left)
					note.flick = FlickType::Right;
				else if (note.flick == FlickType::Right)
					note.flick = FlickType::Left;
			}
		}

		pasteData.pasting = !pasteData.notes.empty();
		if (pasteData.pasting)
		{
			// find the lane in which the cursor is in the middle of pasted notes
			int left = MAX_LANE;
			int right = MIN_LANE;
			int leftmostLane = MAX_LANE;
			int rightmostLane = MIN_LANE;
			int minTick = std::numeric_limits<int>::max();
			for (const auto& [_, note] : pasteData.notes)
			{
				leftmostLane = std::min(leftmostLane, note.lane);
				rightmostLane = std::max(rightmostLane, note.lane + note.width - 1);
				left = std::min(left, note.lane + note.width);
				right = std::max(right, note.lane);
				minTick = std::min(minTick, note.tick);
			}

			pasteData.minTick = std::abs(minTick);
			pasteData.minLaneOffset = MIN_LANE - leftmostLane;
			pasteData.maxLaneOffset = MAX_LANE - rightmostLane;
			pasteData.midLane = (left + right) / 2;
		}
	}

	void ScoreContext::confirmPaste()
	{
		Score prev = score;

		// update IDs and copy notes
		for (auto& [_, note] : pasteData.notes)
		{
			note.ID += nextID;
			if (note.parentID != -1)
				note.parentID += nextID;

			note.lane += pasteData.offsetLane;
			note.tick += pasteData.offsetTicks;
			score.notes[note.ID] = note;
		}

		for (auto& [_, hold] : pasteData.holds)
		{
			hold.start.ID += nextID;
			hold.end += nextID;
			for (auto& step : hold.steps)
				step.ID += nextID;

			score.holdNotes[hold.start.ID] = hold;
		}

		// select newly pasted notes
		selectedNotes.clear();
		std::transform(pasteData.notes.begin(), pasteData.notes.end(),
			std::inserter(selectedNotes, selectedNotes.end()),
			[this](const auto& it) { return it.second.ID; });

		nextID += pasteData.notes.size();
		pasteData.pasting = false;
		pushHistory("Paste notes", prev, score);
	}

	void ScoreContext::paste(bool flip)
	{
		const char* clipboardDataPtr = ImGui::GetClipboardText();
		if (clipboardDataPtr == nullptr)
			return;

		std::string clipboardData(clipboardDataPtr);
		if (!startsWith(clipboardData, clipboardSignature))
			return;

		doPasteData(json::parse(clipboardData.substr(strlen(clipboardSignature))), flip);
	}

	void ScoreContext::shrinkSelection(Direction direction)
	{
		if (selectedNotes.size() < 2)
			return;

		Score prev = score;

		std::vector<int> sortedSelection(selectedNotes.begin(), selectedNotes.end());
		std::sort(sortedSelection.begin(), sortedSelection.end(), [this](int a, int b)
		{
			const Note& n1 = score.notes.at(a);
			const Note& n2 = score.notes.at(b);
			return n1.tick == n2.tick ? n1.lane < n2.lane : n1.tick < n2.tick;
		});

		int factor = 1; // tick increment/decrement amount
		if (direction == Direction::Up)
		{
			// start from the last note
			std::reverse(sortedSelection.begin(), sortedSelection.end());
			factor = -1;
		}

		int firstTick = score.notes.at(*sortedSelection.begin()).tick;
		for (int i = 0; i < sortedSelection.size(); ++i)
			score.notes[sortedSelection[i]].tick = firstTick + (i * factor);
		
		for (int id : getHoldsFromSelection())
			sortHold(score, score.holdNotes.at(id));

		pushHistory("Shrink notes", prev, score);
	}

	void ScoreContext::connectHoldsInSelection()
	{
		if (!selectionCanConnect())
			return;

		Score prev = score;
		Note& note1 = score.notes[*selectedNotes.begin()];
		Note& note2 = score.notes[*std::next(selectedNotes.begin())];
		
		Note& earlierNote = note1.getType() == NoteType::HoldEnd ? note1 : note2;
		Note& laterNote = note1.getType() == NoteType::HoldEnd ? note2 : note1;

		HoldNote& earlierHold = score.holdNotes[earlierNote.parentID];
		HoldNote& laterHold = score.holdNotes[laterNote.ID];
		
		earlierHold.end = laterHold.end;
		laterNote.parentID = earlierHold.start.ID;

		// We need to determine whether the new end will be critical
		Note& earlierHoldStart = score.notes.at(earlierHold.start.ID);
		Note& laterHoldEnd = score.notes.at(score.holdNotes.at(laterNote.ID).end);
		if (earlierHoldStart.critical)
			laterHoldEnd.critical = true;
		
		laterHoldEnd.parentID = earlierHold.start.ID;

		selectedNotes.clear();
		
		// The later note has ease data so we'll always create it
		if (earlierNote.tick != laterNote.tick)
		{
			Note earlierNoteAsMid = Note(NoteType::HoldMid, earlierNote.tick, earlierNote.lane, earlierNote.width);
			earlierNoteAsMid.ID = nextID++;
			earlierNoteAsMid.critical = earlierHoldStart.critical;
			earlierNoteAsMid.parentID = earlierHold.start.ID;

			score.notes[earlierNoteAsMid.ID] = earlierNoteAsMid;
			HoldStepType stepType = earlierHold.isGuide() ? HoldStepType::Hidden : HoldStepType::Normal;
			earlierHold.steps.push_back({ earlierNoteAsMid.ID, stepType, EaseType::Linear });

			selectedNotes.insert(earlierNoteAsMid.ID);
		}
		
		Note laterNoteAsMid = Note(NoteType::HoldMid, laterNote.tick, laterNote.lane, laterNote.width);
		laterNoteAsMid.ID = nextID++;
		laterNoteAsMid.critical = earlierHoldStart.critical;
		laterNoteAsMid.parentID = earlierHold.start.ID;
		
		score.notes[laterNoteAsMid.ID] = laterNoteAsMid;
		HoldStepType stepType = earlierHold.isGuide() ? HoldStepType::Hidden : laterHold.start.type;
		earlierHold.steps.push_back({ laterNoteAsMid.ID, stepType, laterHold.start.ease });

		selectedNotes.insert(laterNoteAsMid.ID);

		// Copying the steps last will save us a sort
		for (auto& step : laterHold.steps)
		{
			earlierHold.steps.push_back(step);

			Note& note = score.notes.at(step.ID);
			note.critical = earlierHoldStart.critical;
			note.parentID = earlierHold.start.ID;
		}
		
		score.notes.erase(earlierNote.ID);
		score.notes.erase(laterNote.ID);
		score.holdNotes.erase(laterHold.start.ID);
		pushHistory("Connect holds", prev, score);
	}

	void ScoreContext::splitHoldInSelection()
	{
		if (selectedNotes.size() != 1)
			return;

		Note& note = score.notes[*selectedNotes.begin()];
		if (note.getType() != NoteType::HoldMid)
			return;

		HoldNote& hold = score.holdNotes[note.parentID];

		int pos = findHoldStep(hold, note.ID);
		if (pos == -1)
			return;

		Score prev = score;

		Note holdStart = score.notes.at(hold.start.ID);

		Note newSlideEnd = Note(NoteType::HoldEnd, note.tick, note.lane, note.width);
		newSlideEnd.ID = nextID++;
		newSlideEnd.parentID = hold.start.ID;
		newSlideEnd.critical = note.critical;

		Note newSlideStart = Note(NoteType::Hold, note.tick, note.lane, note.width);
		newSlideStart.ID = nextID++;
		newSlideStart.critical = holdStart.critical;

		HoldNote newHold;
		newHold.end = hold.end;
		newHold.startType = hold.startType;
		newHold.endType = hold.endType;

		Note& slideEnd = score.notes.at(hold.end);
		slideEnd.parentID = newSlideStart.ID;

		hold.end = newSlideEnd.ID;
		newHold.start = { newSlideStart.ID, HoldStepType::Normal, hold.steps[pos].ease };
		newHold.steps.resize(hold.steps.size() - pos - 1);
		
		// Backwards loop to avoid incorrect indices after removal
		for (size_t i = hold.steps.size() - 1; i > static_cast<size_t>(pos); i--)
		{
			const HoldStep& step = hold.steps[i];
			score.notes.at(step.ID).parentID = newSlideStart.ID;
			newHold.steps[i - pos - 1] = step;
			hold.steps.erase(hold.steps.begin() + i);
		}

		hold.steps.pop_back();
		score.notes.erase(note.ID);
		
		selectedNotes.clear();
		selectedNotes.insert(newSlideStart.ID);

		score.notes[newSlideEnd.ID] = newSlideEnd;
		score.notes[newSlideStart.ID] = newSlideStart;
		score.holdNotes[newSlideStart.ID] = newHold;
		pushHistory("Split hold", prev, score);
	}

	void ScoreContext::undo()
	{
		if (history.hasUndo())
		{
			score = history.undo();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
			upToDate = false;

			scoreStats.calculateStats(score);
			scorePreviewDrawData.calculateDrawData(score);
		}
	}

	void ScoreContext::redo()
	{
		if (history.hasRedo())
		{
			score = history.redo();
			clearSelection();

			UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
			upToDate = false;

			scoreStats.calculateStats(score);
			scorePreviewDrawData.calculateDrawData(score);
		}
	}

	void ScoreContext::pushHistory(std::string description, const Score& prev, const Score& curr)
	{
		history.pushHistory(description, prev, curr);

		UI::setWindowTitle((workingData.filename.size() ? File::getFilename(workingData.filename) : windowUntitled) + "*");
		scoreStats.calculateStats(score);
		scorePreviewDrawData.calculateDrawData(score);

		upToDate = false;
	}

	int ScoreContext::minTickFromSelection() const
	{
		int minTick = score.notes.at(*std::min_element(selectedNotes.begin(), selectedNotes.end(),
			[this](int id1, int id2)
		{	return noteExists(id1, score) && noteExists(id2, score) &&
				score.notes.at(id1).tick < score.notes.at(id2).tick;
		})).tick;

		std::unordered_set<int> selectedHolds = getHoldsFromSelection();
		return selectedHolds.empty() ?
			minTick : std::min(minTick, score.notes.at(*std::min_element(selectedHolds.begin(), selectedHolds.end(),
			[this](int id1, int id2)
		{	return noteExists(id1, score) && noteExists(id2, score) &&
				score.notes.at(id1).tick < score.notes.at(id2).tick;
		})).tick);
	}

	bool ScoreContext::selectionHasEase() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(), [this](int id)
			{ return noteExists(id, score) && score.notes.at(id).hasEase(); });
	}

	bool ScoreContext::selectionHasStep() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(), [this](int id)
		{
			return !CommonNoteFilters::guideFilter()->isGuideHold(id, score) &&
				score.notes.at(id).getType() == NoteType::HoldMid;
		});
	}

	bool ScoreContext::selectionHasAnyStep() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
			[this](int id) { return score.notes.at(id).getType() == NoteType::HoldMid; });
	}

	bool ScoreContext::selectionHasFlickable() const
	{
		return std::any_of(selectedNotes.begin(), selectedNotes.end(),
			[this](int id) { return CommonNoteFilters::flickableFilter()->canFlick(id, score); });
	}

	bool ScoreContext::selectionCanConnect() const
	{
		if (selectedNotes.size() != 2)
			return false;

		const auto& note1 = score.notes.at(*selectedNotes.begin());
		const auto& note2 = score.notes.at(*std::next(selectedNotes.begin()));
		if (note1.tick == note2.tick)
			return (note1.getType() == NoteType::Hold && note2.getType() == NoteType::HoldEnd) ||
						 (note1.getType() == NoteType::HoldEnd && note2.getType() == NoteType::Hold);
		
		auto noteTickCompareFunc = [](const Note& n1, const Note& n2) { return n1.tick < n2.tick; };
		Note earlierNote = std::min(note1, note2, noteTickCompareFunc);
		Note laterNote = std::max(note1, note2, noteTickCompareFunc);

		if (earlierNote.getType() != NoteType::HoldEnd || laterNote.getType() != NoteType::Hold)
			return false;

		return (score.holdNotes.at(earlierNote.parentID).isGuide() == score.holdNotes.at(laterNote.ID).isGuide());
	}
	
	bool ScoreContext::selectionCanChangeHoldType() const
	{
		static auto pred = [this](int id)
		{
			const auto it = score.notes.find(id);
			if (it == score.notes.end())
				return false;

			return it->second.getType() == NoteType::Hold || it->second.getType() == NoteType::HoldEnd;
		};
		static CustomFilter holdsFilter(pred);
		
		return !AggregateNotesFilter()
			.add(&holdsFilter)
			.add(&inverseGuideFilter)
			.filter(selectedNotes, score)
			.empty();
	}
}
